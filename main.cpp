#include "Pokitto.h"
extern "C"
{
    #include "reciter.h"
    #include "sam.h"
    #include "debug.h"
}
Pokitto::Core game;

/**************************************************************************/
#define HELD 0
#define NEW 1
#define RELEASE 2
byte CompletePad, ExPad, TempPad, myPad;
bool _A[3], _B[3], _C[3], _Up[3], _Down[3], _Left[3], _Right[3];

DigitalIn _aPin(P1_9);
DigitalIn _bPin(P1_4);
DigitalIn _cPin(P1_10);
DigitalIn _upPin(P1_13);
DigitalIn _downPin(P1_3);
DigitalIn _leftPin(P1_25);
DigitalIn _rightPin(P1_7);

void UPDATEPAD(int pad, int var) {
  _C[pad] = (var >> 1)&1;
  _B[pad] = (var >> 2)&1;
  _A[pad] = (var >> 3)&1;
  _Down[pad] = (var >> 4)&1;
  _Left[pad] = (var >> 5)&1;
  _Right[pad] = (var >> 6)&1;
  _Up[pad] = (var >> 7)&1;
}

void UpdatePad(int joy_code){
  ExPad = CompletePad;
  CompletePad = joy_code;
  UPDATEPAD(HELD, CompletePad); // held
  UPDATEPAD(RELEASE, (ExPad & (~CompletePad))); // released
  UPDATEPAD(NEW, (CompletePad & (~ExPad))); // newpress
}

byte updateButtons(byte var){
   var = 0;
   if (_cPin) var |= (1<<1);
   if (_bPin) var |= (1<<2);
   if (_aPin) var |= (1<<3); // P1_9 = A
   if (_downPin) var |= (1<<4);
   if (_leftPin) var |= (1<<5);
   if (_rightPin) var |= (1<<6);
   if (_upPin) var |= (1<<7);

   return var;
}






void WriteWav(char* filename, char* buffer, int bufferlength)
{
	FILE *file = fopen(filename, "wb");
	if (file == NULL) return;
	//RIFF header
	fwrite("RIFF", 4, 1,file);
	unsigned int filesize=bufferlength + 12 + 16 + 8 - 8;
	fwrite(&filesize, 4, 1, file);
	fwrite("WAVE", 4, 1, file);

	//format chunk
	fwrite("fmt ", 4, 1, file);
	unsigned int fmtlength = 16;
	fwrite(&fmtlength, 4, 1, file);
	unsigned short int format=1; //PCM
	fwrite(&format, 2, 1, file);
	unsigned short int channels=1;
	fwrite(&channels, 2, 1, file);
	unsigned int samplerate = 22050;
	fwrite(&samplerate, 4, 1, file);
	fwrite(&samplerate, 4, 1, file); // bytes/second
	unsigned short int blockalign = 1;
	fwrite(&blockalign, 2, 1, file);
	unsigned short int bitspersample=8;
	fwrite(&bitspersample, 2, 1, file);

	//data chunk
	fwrite("data", 4, 1, file);
	fwrite(&bufferlength, 4, 1, file);
	fwrite(buffer, bufferlength, 1, file);

	fclose(file);
}

void PrintUsage()
{
	printf("usage: sam [options] Word1 Word2 ....\n");
	printf("options\n");
	printf("	-phonetic 		enters phonetic mode. (see below)\n");
	printf("	-pitch number		set pitch value (default=64)\n");
	printf("	-speed number		set speed value (default=72)\n");
	printf("	-throat number		set throat value (default=128)\n");
	printf("	-mouth number		set mouth value (default=128)\n");
	printf("	-wav filename		output to wav instead of libsdl\n");
	printf("	-sing			special treatment of pitch\n");
	printf("	-debug			print additional debug messages\n");
	printf("\n");


	printf("     VOWELS                            VOICED CONSONANTS	\n");
	printf("IY           f(ee)t                    R        red		\n");
	printf("IH           p(i)n                     L        allow		\n");
	printf("EH           beg                       W        away		\n");
	printf("AE           Sam                       W        whale		\n");
	printf("AA           pot                       Y        you		\n");
	printf("AH           b(u)dget                  M        Sam		\n");
	printf("AO           t(al)k                    N        man		\n");
	printf("OH           cone                      NX       so(ng)		\n");
	printf("UH           book                      B        bad		\n");
	printf("UX           l(oo)t                    D        dog		\n");
	printf("ER           bird                      G        again		\n");
	printf("AX           gall(o)n                  J        judge		\n");
	printf("IX           dig(i)t                   Z        zoo		\n");
	printf("				       ZH       plea(s)ure	\n");
	printf("   DIPHTHONGS                          V        seven		\n");
	printf("EY           m(a)de                    DH       (th)en		\n");
	printf("AY           h(igh)						\n");
	printf("OY           boy						\n");
	printf("AW           h(ow)                     UNVOICED CONSONANTS	\n");
	printf("OW           slow                      S         Sam		\n");
	printf("UW           crew                      Sh        fish		\n");
	printf("                                       F         fish		\n");
	printf("                                       TH        thin		\n");
	printf(" SPECIAL PHONEMES                      P         poke		\n");
	printf("UL           sett(le) (=AXL)           T         talk		\n");
	printf("UM           astron(omy) (=AXM)        K         cake		\n");
	printf("UN           functi(on) (=AXN)         CH        speech		\n");
	printf("Q            kitt-en (glottal stop)    /H        a(h)ead	\n");
}




#ifdef USESDL

int pos = 0;
void MixAudio(void *unused, Uint8 *stream, int len)
{
	int bufferpos = GetBufferLength();
	char *buffer = GetBuffer();
	int i;
	if (pos >= bufferpos) return;
	if ((bufferpos-pos) < len) len = (bufferpos-pos);
	for(i=0; i<len; i++)
	{
		stream[i] = buffer[pos];
		pos++;
	}
}


void OutputSound()
{
	int bufferpos = GetBufferLength();
	bufferpos /= 50;
	SDL_AudioSpec fmt;

	fmt.freq = 22050;
	fmt.format = AUDIO_U8;
	fmt.channels = 1;
	fmt.samples = 2048;
	fmt.callback = MixAudio;
	fmt.userdata = NULL;

	/* Open the audio device and start playing sound! */
	if ( SDL_OpenAudio(&fmt, NULL) < 0 )
	{
		printf("Unable to open audio: %s\n", SDL_GetError());
		exit(1);
	}
	SDL_PauseAudio(0);
	//SDL_Delay((bufferpos)/7);

	while (pos < bufferpos)
	{
		SDL_Delay(100);
	}

	SDL_CloseAudio();
}

#else

void OutputSound() {}

#endif

int debug = 0;







int main(){

    game.begin();
    game.display.width = 220; // full size
    game.display.height = 174;


	int i;
	int phonetic = 0;

	char* wavfilename = "speech.wav";//NULL;
	char input[256];

	for(i=0; i<256; i++) input[i] = 0; // just in case

/*

//	for(i=0; input[i] != 0; i++)
//		input[i] = toupper((int)input[i]);

//sam -mouth 150 -throat 200 -pitch 58 -speed 120 "Eee Tea! Phone home."


    phonetic = 1;
    SetPitch(64);
    SetSpeed(72);
    strcat(input,"/HEH4LOW DHEHR. YUW4 NOW MIY4Y, AY4 AEM SAE5M,");

	if (!phonetic)
	{
		strcat(input, "[");
		if (!TextToPhonemes(input)) return 1;
		if (debug)
			printf("phonetic input: %s\n", input);
	} else strcat(input, "\x9b");


	SetInput(input);
	if (!SAMMain())
	{
		PrintUsage();
		return 1;
	}

	if (wavfilename != NULL)
		WriteWav(wavfilename, GetBuffer(), GetBufferLength()/50);
	else
		OutputSound();

*/

    while (game.isRunning()) {

    if(game.update()){
        game.display.setCursor(0,0);
        game.display.color=1;
        game.display.print("Hello World!");

        myPad = updateButtons(myPad);
        UpdatePad(myPad);

     //   if(_B[HELD]){
            phonetic = 0;
            SetMouth(150);
            SetThroat(200);
            SetPitch(58);
            SetSpeed(120);
            game.display.print("strcat text");
            strcat(input,"Eee Tea! Phone home.");

            if (!phonetic){
                game.display.print("[");
                strcat(input, "[");
                if (!TextToPhonemes(input)){game.display.print("TextToPhonemes Failed");}
            } else {
                game.display.print("phonetic");
                strcat(input, "\x9b");
            }
                game.display.print("So Far So Good!");

            game.display.print("SetInput");
            SetInput(input);
            game.display.print("SetInput OK");
/*

            if (!SAMMain()){
                game.display.print("SAMMain Failed");
            }
            game.display.print("SAMMain OK");

            if (wavfilename != NULL){
                game.display.print(wavfilename);
                WriteWav(wavfilename, GetBuffer(), GetBufferLength()/50);
            }else{
                game.display.print("OutputSound");
                OutputSound();
            }
  //      }
*/
    }

  }
    return 1;
}
