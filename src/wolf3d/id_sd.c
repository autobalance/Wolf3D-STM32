//
//      ID Engine
//      ID_SD.c - Sound Manager for Wolfenstein 3D
//      v1.2
//      By Jason Blochowiak
//

//
//      This module handles dealing with generating sound on the appropriate
//              hardware
//
//      Depends on: User Mgr (for parm checking)
//
//      Globals:
//              For User Mgr:
//                      SoundBlasterPresent - SoundBlaster card present?
//                      AdLibPresent - AdLib card present?
//                      SoundMode - What device is used for sound effects
//                              (Use SM_SetSoundMode() to set)
//                      MusicMode - What device is used for music
//                              (Use SM_SetMusicMode() to set)
//                      DigiMode - What device is used for digitized sound effects
//                              (Use SM_SetDigiDevice() to set)
//
//              For Cache Mgr:
//                      NeedsDigitized - load digitized sounds?
//                      NeedsMusic - load music?
//

#include "wl_def.h"
#include "fmopl.h"

#define ORIGSAMPLERATE 7042

typedef struct
{
    char RIFF[4];
    longword filelenminus8;
    char WAVE[4];
    char fmt_[4];
    longword formatlen;
    word val0x0001;
    word channels;
    longword samplerate;
    longword bytespersec;
    word bytespersample;
    word bitspersample;
} headchunk;

typedef struct
{
    char chunkid[4];
    longword chunklength;
} wavechunk;

typedef struct
{
    uint32_t startpage;
    uint32_t length;
} digiinfo;

//static Mix_Chunk *SoundChunks[ STARTMUSIC - STARTDIGISOUNDS];
static byte      *SoundBuffers[STARTMUSIC - STARTDIGISOUNDS];

globalsoundpos channelSoundPos[/*MIX_CHANNELS*/0];

//      Global variables
        boolean         AdLibPresent,
                        SoundBlasterPresent,SBProPresent,
                        SoundPositioned;
        SDMode          SoundMode;
        SMMode          MusicMode;
        SDSMode         DigiMode;
static  byte          **SoundTable;
        int             DigiMap[LASTSOUND];
        int             DigiChannel[STARTMUSIC - STARTDIGISOUNDS];

//      Internal variables
static  boolean                 SD_Started;
static  boolean                 nextsoundpos;
static  soundnames              SoundNumber;
static  soundnames              DigiNumber;
static  word                    SoundPriority;
static  word                    DigiPriority;
static  int                     LeftPosition;
static  int                     RightPosition;

        word                    NumDigi;
static  digiinfo               *DigiList;
static  boolean                 DigiPlaying;

//      PC Sound variables

static  byte * volatile         pcSound;


//      AdLib variables
static  byte * volatile         alSound;
static  byte                    alBlock;
static  longword                alLengthLeft;
static  longword                alTimeCount;
static  Instrument              alZeroInst;

//      Sequencer variables
static  volatile boolean        sqActive;
static  word                   *sqHack;
static  word                   *sqHackPtr;
static  int                     sqHackLen;
static  int                     sqHackSeqLen;
static  longword                sqHackTime;

// TODO: Get multiple channels of digi-sounds working
//       (a BusFault incurs somewhat randomly if we use more than 1 right now)
#define N_DIGICHANS 1

volatile uint8_t *DigiChan_PagePtr[N_DIGICHANS];
volatile int DigiChan_Size[N_DIGICHANS];
volatile int DigiChan_PosScale[N_DIGICHANS];
volatile int DigiChan_Playing[N_DIGICHANS];

uint32_t GetTicks()
{
    return (uint32_t) get_ticks_ms();
}

inline void Delay(int wolfticks)
{
    if(wolfticks>0) delay_ms(wolfticks * 100 / 7);
}


static void SDL_SoundFinished(void)
{
    SoundNumber   = (soundnames)0;
    SoundPriority = 0;
}

void
SD_StopDigitized(void)
{
    DigiPlaying = false;
    DigiNumber = (soundnames) 0;
    DigiPriority = 0;
    SoundPositioned = false;
    if ((DigiMode == sds_PC) && (SoundMode == sdm_PC))
        SDL_SoundFinished();

    switch (DigiMode)
    {
        case sds_PC:
//            SDL_PCStopSampleInIRQ();
            break;
        case sds_SoundBlaster:
//            SDL_SBStopSampleInIRQ();
            for (int ch = 0; ch < N_DIGICHANS; ch++)
            {
                DigiChan_Playing[ch] = 0;
            }
            break;
    }
}

// TODO: Get multiple channels of digi-sounds working
//       (a BusFault incurs somewhat randomly if we use more than 1 right now)
int SD_GetChannelForDigi(int which)
{
    // channel 0 and 1 are reserved for player weapon and boss weapon, respectively
    /*if(DigiChannel[which] != -1) return DigiChannel[which];

    int DigiChan_OpenSlot = 1;
    while (++DigiChan_OpenSlot < N_DIGICHANS && DigiChan_Playing[DigiChan_OpenSlot]);

    return DigiChan_OpenSlot;*/

    return 0;
}

void SD_SetPosition(int channel, int leftpos, int rightpos)
{
    if((leftpos < 0) || (leftpos > 15) || (rightpos < 0) || (rightpos > 15)
            || ((leftpos == 15) && (rightpos == 15)))
        Quit("SD_SetPosition: Illegal position");

    switch (DigiMode)
    {
        case sds_SoundBlaster:
            // not quite logarithmic volume scaling (divide by 4), but good enough
            DigiChan_PosScale[channel] = (leftpos + rightpos) / 2 / 4 + 1;
            break;
    }
}

void SD_PrepareSound(int which)
{
    if(DigiList == NULL)
        Quit("SD_PrepareSound(%i): DigiList not initialized!\n", which);

    int page = DigiList[which].startpage;
    int size = DigiList[which].length;

    byte *origsamples = PM_GetSound(page);
    if(origsamples + size >= PM_GetEnd())
        Quit("SD_PrepareSound(%i): Sound reaches out of page file!\n", which);
}

// produce 4 samples of AdLib sound every interrupt
#define ADLIB_IRQ_SAMPLES 4
volatile int adlib_curr_sample = 0;
volatile int16_t adlib_samples[DAC_BUF_LEN];

// Use Timer 7 to run 7042/3 ~= 2347 interrupts per second to produce 4 AdLib samples per interrupt.
// Profiling shows that SDL_IMFMusicPlayer takes about 6000 CPU cycles to produce 1 sample, so
// at the current rate of interrupts we have about 6000*4 = 24000 CPU cycles per interrupt, or
// about 24000/480MHz = 50us per interrupt at 480MHz core clock which produces good results.
//
// TODO: Improve interrupt latency at lower frequencies such as 60MHz or 120MHz core clocks.
//       AdLib emulation is somewhat taxing, and at e.g. 60MHz core clock we require 25ms to compute
//       all 256 DAC samples (which fit in a 256/7042 ~= 0.036s = 36ms window to produce all the samples).
//       So this may require the use of a different OPL emulator such as DosBox OPL/dbopl
//       (more than twice as fast as fmopl from testing in Chocolate-Wolfenstein-3D on PC).
void adlib_irq_setup(void)
{
    RCC->APB1LENR |= RCC_APB1LENR_TIM7EN;

    NVIC_EnableIRQ(TIM7_IRQn);

    TIM7->PSC = SystemD2Clock / 70420 - 1;
    TIM7->ARR = 30 - 1;
    TIM7->CR2 = 0b010 << TIM_CR2_MMS_Pos;
    TIM7->CR1 = TIM_CR1_ARPE | TIM_CR1_URS;
    TIM7->DIER |= TIM_DIER_UIE;
    TIM7->CR1 |= TIM_CR1_CEN;
}

void TIM7_IRQHandler(void)
{
    NVIC_DisableIRQ(TIM7_IRQn);
    TIM7->SR = 0;

    if ((SoundMode == sdm_AdLib) || (MusicMode == smm_AdLib))
    {
        uint8_t music[ADLIB_IRQ_SAMPLES*4];
        memset(music, 0, ADLIB_IRQ_SAMPLES*4);
        SDL_IMFMusicPlayer(NULL, music, ADLIB_IRQ_SAMPLES*4);

        for (int i = 0; i < ADLIB_IRQ_SAMPLES*4; i+=4)
        {
            int16_t left = ((uint16_t) music[i]) | (((uint16_t) music[i+1]) << 8);
            int16_t right = ((uint16_t) music[i+2]) | (((uint16_t) music[i+3]) << 8);
            adlib_samples[adlib_curr_sample + i/4] = (left + right) / (2 * 16);
        }

        adlib_curr_sample += ADLIB_IRQ_SAMPLES;

        if (adlib_curr_sample == DAC_BUF_LEN)
        {
            adlib_curr_sample = 0;
            TIM7->CR1 &= ~TIM_CR1_CEN;
            return;
        }
    }

    NVIC_EnableIRQ(TIM7_IRQn);
}

extern uint16_t dac_buf[2][DAC_BUF_LEN];
void DMA1_Stream1_IRQHandler(void)
{
    DMA1->LIFCR = 0xF40;

    uint32_t ct = (DMA1_Stream1->CR & DMA_SxCR_CT_Msk) >> DMA_SxCR_CT_Pos;

    int16_t dac_mix[DAC_BUF_LEN];
    memset(dac_mix, 0, DAC_BUF_LEN*sizeof(int16_t));

    if ((SoundMode == sdm_AdLib) || (MusicMode == smm_AdLib))
    {
        /*uint8_t music[DAC_BUF_LEN*4];
        memset(music, 0, DAC_BUF_LEN*4);
        SDL_IMFMusicPlayer(NULL, music, DAC_BUF_LEN*4);

        for (int i = 0; i < DAC_BUF_LEN*4; i+=4)
        {
            int16_t left = ((uint16_t) music[i]) | (((uint16_t) music[i+1]) << 8);
            int16_t right = ((uint16_t) music[i+2]) | (((uint16_t) music[i+3]) << 8);
            dac_mix[i/4] = (left + right) / (2 * 16);
        }*/
        memcpy(dac_mix, adlib_samples, DAC_BUF_LEN*sizeof(uint16_t));
        adlib_irq_setup();
    }

    for (int ch = 0; ch < N_DIGICHANS; ch++)
    {
        if (DigiChan_Playing[ch])
        {
            // determine if this is the last group of bytes in the digi-sound
            int sizetocopy = DigiChan_Size[ch] < DAC_BUF_LEN ? DigiChan_Size[ch] : DAC_BUF_LEN;
            for (int i = 0; i < sizetocopy; i++)
            {
                dac_mix[i] += (((int16_t) DigiChan_PagePtr[ch][i] - 128) * 16) / DigiChan_PosScale[ch];
            }
            // continue last byte in sound to avoid popping from sharp transition
            for (int i = sizetocopy; i < DAC_BUF_LEN; i++)
            {
                dac_mix[i] += (((int16_t) DigiChan_PagePtr[ch][sizetocopy-1] - 128) * 16) / DigiChan_PosScale[ch];
            }

            DigiChan_Size[ch] -= sizetocopy;
            DigiChan_PagePtr[ch] += sizetocopy;

            if (DigiChan_Size[ch] <= 0)
            {
                DigiChan_Playing[ch] = 0;
            }
        }
    }

    // TODO: implement a better compressor/limiter than hard clipping once multi-channel sound is working
    for (int i = 0; i < DAC_BUF_LEN; i++)
    {
        dac_mix[i] = dac_mix[i] > 2047 ? 2047 : dac_mix[i];
        dac_mix[i] = dac_mix[i] < -2048 ? -2048 : dac_mix[i];
        dac_buf[1-ct][i] = dac_mix[i] + 2048;
    }

    SCB_CleanDCache_by_Addr((uint32_t*)(((uint32_t)dac_buf[1-ct]) & ~(uint32_t)0x1F), DAC_BUF_LEN*sizeof(uint16_t) + 32);
}

int SD_PlayDigitized(word which,int leftpos,int rightpos)
{
    if (!DigiMode)
        return 0;

    if (which >= NumDigi)
        Quit("SD_PlayDigitized: bad sound number %i", which);

    int channel = SD_GetChannelForDigi(which);
    if (channel >= N_DIGICHANS)
    {
        return 0;
    }

    SD_SetPosition(channel, leftpos,rightpos);

    DigiPlaying = true;

    int page = DigiList[which].startpage;
    int size = DigiList[which].length;

    DigiChan_PagePtr[channel] = PM_GetSound(page);
    DigiChan_Size[channel] = size;
    DigiChan_Playing[channel] = 1;

    return channel;
}

void SD_ChannelFinished(int channel)
{
    channelSoundPos[channel].valid = 0;
}

void
SD_SetDigiDevice(SDSMode mode)
{
    boolean devicenotpresent;

    if (mode == DigiMode)
        return;

    SD_StopDigitized();

    devicenotpresent = false;
    switch (mode)
    {
        case sds_SoundBlaster:
            if (!SoundBlasterPresent)
                devicenotpresent = true;
            break;
    }

    if (!devicenotpresent)
    {
        DigiMode = mode;

    }
}

void
SDL_SetupDigi(void)
{
    // Correct padding enforced by PM_Startup()
    word *soundInfoPage = (word *) (void *) PM_GetPage(ChunksInFile-1);
    NumDigi = (word) PM_GetPageSize(ChunksInFile - 1) / 4;

    DigiList = (digiinfo *) malloc(NumDigi * sizeof(digiinfo));
    int i;
    for(i = 0; i < NumDigi; i++)
    {
        // Calculate the size of the digi from the sizes of the pages between
        // the start page and the start page of the next sound

        DigiList[i].startpage = soundInfoPage[i * 2];
        if((int) DigiList[i].startpage >= ChunksInFile - 1)
        {
            NumDigi = i;
            break;
        }

        int lastPage;
        if(i < NumDigi - 1)
        {
            lastPage = soundInfoPage[i * 2 + 2];
            if(lastPage == 0 || lastPage + PMSoundStart > ChunksInFile - 1) lastPage = ChunksInFile - 1;
            else lastPage += PMSoundStart;
        }
        else lastPage = ChunksInFile - 1;

        int size = 0;
        for(int page = PMSoundStart + DigiList[i].startpage; page < lastPage; page++)
            size += PM_GetPageSize(page);

        // Don't include padding of sound info page, if padding was added
        if(lastPage == ChunksInFile - 1 && PMSoundInfoPagePadded) size--;

        // Patch lower 16-bit of size with size from sound info page.
        // The original VSWAP contains padding which is included in the page size,
        // but not included in the 16-bit size. So we use the more precise value.
        if((size & 0xffff0000) != 0 && (size & 0xffff) < soundInfoPage[i * 2 + 1])
            size -= 0x10000;
        size = (size & 0xffff0000) | soundInfoPage[i * 2 + 1];

        DigiList[i].length = size;
    }

    for(i = 0; i < LASTSOUND; i++)
    {
        DigiMap[i] = -1;
        DigiChannel[i] = -1;
    }
}

//      AdLib Code

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ALStopSound() - Turns off any sound effects playing through the
//              AdLib card
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_ALStopSound(void)
{
    alSound = 0;
    alOut(alFreqH + 0, 0);
}

static void
SDL_AlSetFXInst(Instrument *inst)
{
    byte c,m;

    m = 0;      // modulator cell for channel 0
    c = 3;      // carrier cell for channel 0
    alOut(m + alChar,inst->mChar);
    alOut(m + alScale,inst->mScale);
    alOut(m + alAttack,inst->mAttack);
    alOut(m + alSus,inst->mSus);
    alOut(m + alWave,inst->mWave);
    alOut(c + alChar,inst->cChar);
    alOut(c + alScale,inst->cScale);
    alOut(c + alAttack,inst->cAttack);
    alOut(c + alSus,inst->cSus);
    alOut(c + alWave,inst->cWave);

    // Note: Switch commenting on these lines for old MUSE compatibility
//    alOutInIRQ(alFeedCon,inst->nConn);
    alOut(alFeedCon,0);
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ALPlaySound() - Plays the specified sound on the AdLib card
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_ALPlaySound(AdLibSound *sound)
{
    Instrument      *inst;
    byte            *data;

    SDL_ALStopSound();

    alLengthLeft = sound->common.length;
    data = sound->data;
    alBlock = ((sound->block & 7) << 2) | 0x20;
    inst = &sound->inst;

    if (!(inst->mSus | inst->cSus))
    {
        Quit("SDL_ALPlaySound() - Bad instrument");
    }

    SDL_AlSetFXInst(inst);
    alSound = (byte *)data;
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ShutAL() - Shuts down the AdLib card for sound effects
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_ShutAL(void)
{
    alSound = 0;
    alOut(alEffects,0);
    alOut(alFreqH + 0,0);
    SDL_AlSetFXInst(&alZeroInst);
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_CleanAL() - Totally shuts down the AdLib card
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_CleanAL(void)
{
    int     i;

    alOut(alEffects,0);
    for (i = 1; i < 0xf5; i++)
        alOut(i, 0);
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_StartAL() - Starts up the AdLib card for sound effects
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_StartAL(void)
{
    alOut(alEffects, 0);
    SDL_AlSetFXInst(&alZeroInst);
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_DetectAdLib() - Determines if there's an AdLib (or SoundBlaster
//              emulating an AdLib) present
//
///////////////////////////////////////////////////////////////////////////
static boolean
SDL_DetectAdLib(void)
{
    for (int i = 1; i <= 0xf5; i++)       // Zero all the registers
        alOut(i, 0);

    alOut(1, 0x20);             // Set WSE=1
//    alOut(8, 0);                // Set CSM=0 & SEL=0

    return true;
}

////////////////////////////////////////////////////////////////////////////
//
//      SDL_ShutDevice() - turns off whatever device was being used for sound fx
//
////////////////////////////////////////////////////////////////////////////
static void
SDL_ShutDevice(void)
{
    switch (SoundMode)
    {
        case sdm_PC:
//            SDL_ShutPC();
            break;
        case sdm_AdLib:
            SDL_ShutAL();
            break;
    }
    SoundMode = sdm_Off;
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_CleanDevice() - totally shuts down all sound devices
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_CleanDevice(void)
{
    if ((SoundMode == sdm_AdLib) || (MusicMode == smm_AdLib))
        SDL_CleanAL();
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_StartDevice() - turns on whatever device is to be used for sound fx
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_StartDevice(void)
{
    switch (SoundMode)
    {
        case sdm_AdLib:
            SDL_StartAL();
            break;
    }
    SoundNumber = (soundnames) 0;
    SoundPriority = 0;
}

//      Public routines

///////////////////////////////////////////////////////////////////////////
//
//      SD_SetSoundMode() - Sets which sound hardware to use for sound effects
//
///////////////////////////////////////////////////////////////////////////
boolean
SD_SetSoundMode(SDMode mode)
{
    boolean result = false;
    word    tableoffset;

    SD_StopSound();

    if ((mode == sdm_AdLib) && !AdLibPresent)
        mode = sdm_PC;

    switch (mode)
    {
        case sdm_Off:
            tableoffset = STARTADLIBSOUNDS;
            result = true;
            break;
        case sdm_PC:
            tableoffset = STARTPCSOUNDS;
            result = true;
            break;
        case sdm_AdLib:
            tableoffset = STARTADLIBSOUNDS;
            if (AdLibPresent)
                result = true;
            break;
        default:
            Quit("SD_SetSoundMode: Invalid sound mode %i", mode);
            return false;
    }
    SoundTable = &audiosegs[tableoffset];

    if (result && (mode != SoundMode))
    {
        SDL_ShutDevice();
        SoundMode = mode;
        SDL_StartDevice();
    }

    return(result);
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_SetMusicMode() - sets the device to use for background music
//
///////////////////////////////////////////////////////////////////////////
boolean
SD_SetMusicMode(SMMode mode)
{
    boolean result = false;

    SD_FadeOutMusic();
    while (SD_MusicPlaying())
        delay_ms(5);

    switch (mode)
    {
        case smm_Off:
            result = true;
            break;
        case smm_AdLib:
            if (AdLibPresent)
                result = true;
            break;
    }

    if (result)
        MusicMode = mode;

//    SDL_SetTimerSpeed();

    return(result);
}

int numreadysamples = 0;
byte *curAlSound = 0;
byte *curAlSoundPtr = 0;
longword curAlLengthLeft = 0;
int soundTimeCounter = 5;
int samplesPerMusicTick;

void SDL_IMFMusicPlayer(void *udata, uint8_t *stream, int len)
{
    int stereolen = len>>1;
    int sampleslen = stereolen>>1;
    INT16 *stream16 = (INT16 *) (void *) stream;    // expect correct alignment

    while(1)
    {
        if(numreadysamples)
        {
            if(numreadysamples<sampleslen)
            {
                YM3812UpdateOne(0, stream16, numreadysamples);
                stream16 += numreadysamples*2;
                sampleslen -= numreadysamples;
            }
            else
            {
                YM3812UpdateOne(0, stream16, sampleslen);
                numreadysamples -= sampleslen;
                return;
            }
        }
        soundTimeCounter--;
        if(!soundTimeCounter)
        {
            soundTimeCounter = 5;
            if(curAlSound != alSound)
            {
                curAlSound = curAlSoundPtr = alSound;
                curAlLengthLeft = alLengthLeft;
            }
            if(curAlSound)
            {
                if(*curAlSoundPtr)
                {
                    alOut(alFreqL, *curAlSoundPtr);
                    alOut(alFreqH, alBlock);
                }
                else alOut(alFreqH, 0);
                curAlSoundPtr++;
                curAlLengthLeft--;
                if(!curAlLengthLeft)
                {
                    curAlSound = alSound = 0;
                    SoundNumber = (soundnames) 0;
                    SoundPriority = 0;
                    alOut(alFreqH, 0);
                }
            }
        }
        if(sqActive)
        {
            do
            {
                if(sqHackTime > alTimeCount) break;
                sqHackTime = alTimeCount + *(sqHackPtr+1);
                alOut(*(byte *) sqHackPtr, *(((byte *) sqHackPtr)+1));
                sqHackPtr += 2;
                sqHackLen -= 4;
            }
            while(sqHackLen>0);
            alTimeCount++;
            if(!sqHackLen)
            {
                sqHackPtr = sqHack;
                sqHackLen = sqHackSeqLen;
                sqHackTime = 0;
                alTimeCount = 0;
            }
        }
        numreadysamples = samplesPerMusicTick;
    }
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_Startup() - starts up the Sound Mgr
//              Detects all additional sound hardware and installs my ISR
//
///////////////////////////////////////////////////////////////////////////
void
SD_Startup(void)
{
    int     i;

    if (SD_Started)
        return;

    //if(Mix_OpenAudio(param_samplerate, AUDIO_S16, 2, param_audiobuffer))
    //{
    //    printf("Unable to open audio: %s\n", Mix_GetError());
    //    return;
    //}

    //Mix_ReserveChannels(2);  // reserve player and boss weapon channels
    //Mix_GroupChannels(2, MIX_CHANNELS-1, 1); // group remaining channels

    // Init music

    samplesPerMusicTick = param_samplerate / 700;    // SDL_t0FastAsmService played at 700Hz

    if(YM3812Init(1,3579545,param_samplerate))
    {
        printf("Unable to create virtual OPL!!\n");
    }

    for(i=1;i<0xf6;i++)
        YM3812Write(0,i,0);

    YM3812Write(0,1,0x20); // Set WSE=1
//    YM3812Write(0,8,0); // Set CSM=0 & SEL=0       // already set in for statement

    //Mix_HookMusic(SDL_IMFMusicPlayer, 0);
    //Mix_ChannelFinished(SD_ChannelFinished);

    // TODO: implement AdLib support for music
    AdLibPresent = true;
    SoundBlasterPresent = true;

    alTimeCount = 0;

    SD_SetSoundMode(sdm_Off);
    SD_SetMusicMode(smm_Off);

    SDL_SetupDigi();

    adlib_irq_setup();
    dac_start();

    SD_Started = true;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_Shutdown() - shuts down the Sound Mgr
//              Removes sound ISR and turns off whatever sound hardware was active
//
///////////////////////////////////////////////////////////////////////////
void
SD_Shutdown(void)
{
    if (!SD_Started)
        return;

    SD_MusicOff();
    SD_StopSound();

    for(int i = 0; i < STARTMUSIC - STARTDIGISOUNDS; i++)
    {
        //if(SoundChunks[i]) Mix_FreeChunk(SoundChunks[i]);
        if(SoundBuffers[i]) free(SoundBuffers[i]);
    }

    free(DigiList);

    SD_Started = false;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_PositionSound() - Sets up a stereo imaging location for the next
//              sound to be played. Each channel ranges from 0 to 15.
//
///////////////////////////////////////////////////////////////////////////
void
SD_PositionSound(int leftvol,int rightvol)
{
    LeftPosition = leftvol;
    RightPosition = rightvol;
    nextsoundpos = true;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_PlaySound() - plays the specified sound on the appropriate hardware
//
///////////////////////////////////////////////////////////////////////////
boolean
SD_PlaySound(soundnames sound)
{
    boolean         ispos;
    SoundCommon     *s;
    int             lp,rp;

    lp = LeftPosition;
    rp = RightPosition;
    LeftPosition = 0;
    RightPosition = 0;

    ispos = nextsoundpos;
    nextsoundpos = false;

    if (sound == -1 || (DigiMode == sds_Off && SoundMode == sdm_Off))
        return 0;

    s = (SoundCommon *) SoundTable[sound];

    if ((SoundMode != sdm_Off) && !s)
            Quit("SD_PlaySound() - Uncached sound");

    if ((DigiMode != sds_Off) && (DigiMap[sound] != -1))
    {
        if ((DigiMode == sds_PC) && (SoundMode == sdm_PC))
        {
            return 0;
        }
        else
        {

            int channel = SD_PlayDigitized(DigiMap[sound], lp, rp);
            SoundPositioned = ispos;
            DigiNumber = sound;
            DigiPriority = s->priority;
            return channel >= 0;
        }

        return(true);
    }

    if (SoundMode == sdm_Off)
        return 0;

    if (!s->length)
        Quit("SD_PlaySound() - Zero length sound");
    if (s->priority < SoundPriority)
        return 0;

    switch (SoundMode)
    {
        case sdm_PC:
//            SDL_PCPlaySound((PCSound *)s);
            break;
        case sdm_AdLib:
            SDL_ALPlaySound((AdLibSound *)s);
            break;
    }

    SoundNumber = sound;
    SoundPriority = s->priority;

    return 0;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_SoundPlaying() - returns the sound number that's playing, or 0 if
//              no sound is playing
//
///////////////////////////////////////////////////////////////////////////
word
SD_SoundPlaying(void)
{
    boolean result = false;

    switch (SoundMode)
    {
        case sdm_PC:
            result = pcSound? true : false;
            break;
        case sdm_AdLib:
            result = alSound? true : false;
            break;
    }

    if (result)
        return(SoundNumber);
    else
        return(false);
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_StopSound() - if a sound is playing, stops it
//
///////////////////////////////////////////////////////////////////////////
void
SD_StopSound(void)
{
    if (DigiPlaying)
        SD_StopDigitized();

    switch (SoundMode)
    {
        case sdm_PC:
//            SDL_PCStopSound();
            break;
        case sdm_AdLib:
            SDL_ALStopSound();
            break;
    }

    SoundPositioned = false;

    SDL_SoundFinished();
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_WaitSoundDone() - waits until the current sound is done playing
//
///////////////////////////////////////////////////////////////////////////
void
SD_WaitSoundDone(void)
{
    while (SD_SoundPlaying())
        delay_ms(5);
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_MusicOn() - turns on the sequencer
//
///////////////////////////////////////////////////////////////////////////
void
SD_MusicOn(void)
{
    sqActive = true;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_MusicOff() - turns off the sequencer and any playing notes
//      returns the last music offset for music continue
//
///////////////////////////////////////////////////////////////////////////
int
SD_MusicOff(void)
{
    word    i;

    sqActive = false;
    switch (MusicMode)
    {
        case smm_AdLib:
            alOut(alEffects, 0);
            for (i = 0;i < sqMaxTracks;i++)
                alOut(alFreqH + i + 1, 0);
            break;
    }

    return (int) (sqHackPtr-sqHack);
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_StartMusic() - starts playing the music pointed to
//
///////////////////////////////////////////////////////////////////////////
void
SD_StartMusic(int chunk)
{
    SD_MusicOff();

    if (MusicMode == smm_AdLib)
    {
        int32_t chunkLen = CA_CacheAudioChunk(chunk);
        sqHack = (word *)(void *) audiosegs[chunk];     // alignment is correct
        if(*sqHack == 0) sqHackLen = sqHackSeqLen = chunkLen;
        else sqHackLen = sqHackSeqLen = *sqHack++;
        sqHackPtr = sqHack;
        sqHackTime = 0;
        alTimeCount = 0;
        SD_MusicOn();
    }
}

void
SD_ContinueMusic(int chunk, int startoffs)
{
    SD_MusicOff();

    if (MusicMode == smm_AdLib)
    {
        int32_t chunkLen = CA_CacheAudioChunk(chunk);
        sqHack = (word *)(void *) audiosegs[chunk];     // alignment is correct
        if(*sqHack == 0) sqHackLen = sqHackSeqLen = chunkLen;
        else sqHackLen = sqHackSeqLen = *sqHack++;
        sqHackPtr = sqHack;

        if(startoffs >= sqHackLen)
        {
            Quit("SD_StartMusic: Illegal startoffs provided!");
        }

        // fast forward to correct position
        // (needed to reconstruct the instruments)

        for(int i = 0; i < startoffs; i += 2)
        {
            byte reg = *(byte *)sqHackPtr;
            byte val = *(((byte *)sqHackPtr) + 1);
            if(reg >= 0xb1 && reg <= 0xb8) val &= 0xdf;           // disable play note flag
            else if(reg == 0xbd) val &= 0xe0;                     // disable drum flags

            alOut(reg,val);
            sqHackPtr += 2;
            sqHackLen -= 4;
        }
        sqHackTime = 0;
        alTimeCount = 0;

        SD_MusicOn();
    }
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_FadeOutMusic() - starts fading out the music. Call SD_MusicPlaying()
//              to see if the fadeout is complete
//
///////////////////////////////////////////////////////////////////////////
void
SD_FadeOutMusic(void)
{
    switch (MusicMode)
    {
        case smm_AdLib:
            // DEBUG - quick hack to turn the music off
            SD_MusicOff();
            break;
    }
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_MusicPlaying() - returns true if music is currently playing, false if
//              not
//
///////////////////////////////////////////////////////////////////////////
boolean
SD_MusicPlaying(void)
{
    boolean result;

    switch (MusicMode)
    {
        case smm_AdLib:
            result = sqActive;
            break;
        default:
            result = false;
            break;
    }

    return(result);
}
