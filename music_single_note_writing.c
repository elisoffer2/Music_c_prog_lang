//**/** This Is a music synthesizer application used to digitize songs in 8 bit form */

/* lengths of notes:
* 1 - Whole note
* 2 - three quarter note
* 3 - Half note
* 4 - quarter note
* 5 - 8th note
* 6 - 16th note
* 7 - 32nd note
*/

/* Frequency / Octaves of notes
* A1-8
* B1-8
* C1-8
* D1-8
* E1-8
* F1-8
* G1-8
* 
* g#,a#,c#,d#,f#
* ab,bb,db,eb,gb
* v,w,x,y,z
* V1-8 - aflat / gsharp
* W1-8 - bflat / asharp
* X1-8 - dflat / csharp
* Y1-8 - eflat / dsharp
* Z1-8 - gflat / fsharp
*/

/* Priotized Product Backlog*/
// --> sprint this (playing poker)
// -> spaces indicate new lengths combine all the notes together before a space so 10 figures 20 characters betewen spaces allowed
// -> 10 signal array one for each key
// -> extract signal count for each freq note
// 7. multi tones (chord notes / more fingers on piano) (13)
// song bank / playlist
// reduce pause timing
// shuffle play
// refactor code

// cut out for a later time //
// 15. new tones (modify sine wave to square wave or other harmonic types based on instrument sounds)
// 12. mp3 file to file converter
// 3. simplified manual entering of notes (keyboard style)
// 11. song extractor to excel viewer
// 8. excel viewer and song extractor to file
// 9. key piano visualizer and converter
// 10. keyboard attachment to file converter
// 4. live keyboard mode
// 13. song file compressor
// 14. automated piano teacher 

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <io.h>  
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "dsp.h"

// clarity of sound (also scales with test len) 7903 is nyquist sampling rate (double highest frequency on the piano + 1)
#define SAMP_RATE 8000 

// 60 bpm means a quarter note is one second means a whole note is 4 seconds which is 4 times the sampling rate?
// whole note size --> controls tempo
#define TEST_LEN SAMP_RATE*4 //--> max tempo

// maximum size for freqs and lngths (trade off between note count and memory used)
#define BUFF_SIZE 1024

// maximum file size to read in from a song text file
#define FILE_SIZE 65536

// count of bytes taken from a file when copying
#define BUFFERSIZE 512

// PI used for sine wave generation
#define PI (3.14159265)

// more printouts
#define VERBOSE_SETTING 1


int freqTranslator(int * poctave, int * pnote, char noteStr[]) {
	// buffer to frequency converter function
	switch (noteStr[0]) {
		case 'C':
		case 'c':
			*pnote = 0;
			break;
		case 'X':
		case 'x':
			*pnote = 1;
			break;
		case 'D':
		case 'd':
			*pnote = 2;
			break;
		case 'Y':
		case 'y':
			*pnote = 3;
			break;
		case 'E':
		case 'e':
			*pnote = 4;
			break;
		case 'F':
		case 'f':
			*pnote = 5;
			break;
		case 'Z':
		case 'z':
			*pnote = 6;
			break;
		case 'G':
		case 'g':
			*pnote = 7;
			break;
		case 'V':
		case 'v':
			*pnote = 8;
			break;
		case 'A':
		case 'a':
			*pnote = 9;
			break;
		case 'W':
		case 'w':
			*pnote = 10;
			break;
		case 'B':
		case 'b':
			*pnote = 11;
			break;
		case 'P':
		case 'p':
			*poctave = 0;
			*pnote = 0;
			return 0;
		case ' ':
			printf("Found space: Error highest note substitute");
			*poctave = 7;
			*pnote = 11;
			return 1;
			break;
		default:
			printf("Note not recognized. substituting with C1");
			*poctave = 0;
			*pnote = 0;
			return 1;
			break;
	}
	*poctave = (int)noteStr[1] - (int)'0';
	return 0;
}



int main(int argc, char *argv[])
{

	// counters used in the program
	int	i, j, k;

	// signal processing variables
	// MULTINOTE Change here
	char tempoStr[BUFF_SIZE];
	unsigned int tempoControl = TEST_LEN;
	unsigned short signal[tempoControl];	// generated piano signal;

	// frequencies of notes variables
	// MULTINOTE Change here
	int notefreq;
	// MULTINOTE Change here
	char notes[BUFF_SIZE][2];
	// MULTINOTE Change here
	char freqs[BUFF_SIZE];				// note frequencies array

	// length of note variables
	char lngths[BUFF_SIZE];				// note lengths array
	unsigned long signal_length = 0;	// current note signal length scaled from lngths

	// frequencies and length manipulation variables
	char * pnoteStr;
	int r = 0;
	int pnote;
	int poctave;

	// playback settings:
	int startNote = 0;
	int endNote = 0;
	int isLoopSong = 0;
	int isStartLate = 0;
	int isManualEntry = 0;
	
	// input file settings:
	FILE * fp;
	char buffer[BUFFERSIZE];
	char musicFileStr[FILE_SIZE];

	// Table of piano keys frequencies by octave and key
	//						  C		C#/Db D	    D#/Eb E		F	  F#/Gb G	  G#/Ab	A	  A#/Bb B
	int freq_table[8][12] = {{0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0},     // pause notes
							 {33,   35,   37,   39,   41,   44,   46,   49,   52,   55,   58,   62},	// Octave 1
							 {65,   69,   73,   78,   82,   87,   92,   98,   104,  110,  117,  123},	// Octave 2
							 {131,  139,  147,  156,  165,  175,  185,  196,  208,  220,  233,  247},	// Octave 3
							 {262,  277,  294,  311,  330,  349,  370,  392,  415,  440,  466,  494},	// Octave 4
							 {523,  554,  587,  622,  659,  698,  740,  784,  831,  880,  932,  988},	// Octave 5
							 {1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976},	// Octave 6
							 {2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951}};	// Octave 7

	// if there are less than two arguements (runing music alone) then the user needs to input the song
	if (argc < 2)  
	{
		/* Manually entering lengths and tones*/
	MANUAL_TONE_ENTRY:
		isManualEntry = 1;
		printf("\n\nNo Song input into music machine. Please enter Tempo, Lengths and tones: \n\n");
		printf("Enter Lengths:\n\n");
		fgets(lngths, sizeof(lngths), stdin);
		fputs(lngths, stdin);	
		printf("Enter tones\n\n");
		fgets(freqs, sizeof(freqs), stdin);
		fputs(freqs, stdin);
		printf("Enter Tempo:\n\n");
		fgets(tempoStr, sizeof(tempoStr), stdin);
		fputs(tempoStr, stdin);
	}

	// We are loading in a song with specific settings
	else
	{
		isManualEntry = 0;
		strcpy(musicFileStr, "");
		fp = fopen(argv[1], "r"); //so if there was 3 arguements open the file you want to display on stdout
		if ((fp) == NULL)          //if the filepointer is \0 then that file doesn't exist close program
		{
			fprintf(stderr, "File does not exist!\n");
			return (1);
		}

		// start offset
		if (argc > 2)
		{
			//offset is string to long of the byte offset specified by the user. so string of numbers goes to the offset value as a number value integer
			startNote = atoi(argv[2]);
			printf("Start Note Init: %d\n", startNote);
			isStartLate = 1;
		}

		// end offset
		if (argc > 3)
		{
			// and argv[2] is a character array of the integers given which we want to convert to an "integer", but really a long so we can offset that interger value
			endNote = atoi(argv[3]);
			printf("End Note Init: %d\n", endNote);
		}

		// loop the song
		if (argc > 4)
		{
			isLoopSong = atoi(argv[4]);
		}

		//seek for the file. find the file. seek set is the beginning of the file to begin the reading.
		fseek(fp, 0, SEEK_SET); 
		do {
			memset(buffer,0,strlen(buffer));			// clear the buffer so there is no excess when reading the last bit of the file
			i = fread(buffer, 1, BUFFERSIZE, fp);		// read 512 bytes of the file so long and its not the end of file there will be exactly 512 bytes 
			setmode(fileno(stdout), O_BINARY);			// places STDOUT into binary mode fileno is the file number of stdout which is stdout 1
			strcat(musicFileStr, buffer);
			if (fwrite(buffer, 1, i, stdout) != i)		// write to my screen the buffer 512 bytes at a time which is my buffer size and i is the just recently read 512 bytes
			{
				// partial write / read condition
				fprintf(stderr, "Error: Did not write as much as what was read. Amount read: %d", i);
				fclose(fp);
				return (1);
			}
		} while (i == BUFFERSIZE);						//i is updating every while loop by re-reading the file pointed to by *fp when i != Buffersize we reached the end of the file
		
														// space out file read output to stdout:
		printf("\n\n");

		if (VERBOSE_SETTING) {
			printf("\n\n musicFileStr After full Read: \n\n%s\n\n", musicFileStr);
		}
		pnoteStr = strtok(musicFileStr, "\n");
		strcpy(tempoStr, pnoteStr);
		pnoteStr = strtok(NULL, "\n");
		strcpy(freqs, pnoteStr);
		pnoteStr = strtok(NULL, " ,.-");
		strcpy(lngths, pnoteStr);
		strcpy(pnoteStr, "");
	}

	if (VERBOSE_SETTING) {
		printf("\n\nTempo: \n%s", tempoStr);
		printf("\n\nFreqs: \n%s", freqs);
		printf("\n\nLengths: \n%s", lngths);
	}
	
	// Setting the Tempo control
	tempoControl = atoi(tempoStr);
	if (tempoControl > TEST_LEN) {
		fprintf(stderr, "Error: Tempo variable is too large for TEST_LEN: %d", tempoControl);
		exit(1);
	}

	// MULTINOTE Change here
	k = 0;
	if (VERBOSE_SETTING) {
		printf("Splitting string \"%s\" into tokens:\n", freqs);
	}
	pnoteStr = strtok(freqs, " ,.-");
	while (pnoteStr != NULL)
	{
		if (VERBOSE_SETTING) {
			printf("k = %d: %s\n", k, pnoteStr);
		}
		strcpy(notes[k], pnoteStr);
		pnoteStr = strtok(NULL, " ,.-");
		k++;
	}
	
	if (VERBOSE_SETTING) {
		printf("\nlen: %ld\n", strlen(lngths));
		printf("frq: %d\n\n", k);
	}

	if (strlen(lngths) > k) 
	{
		printf("WARNING: Lengths of note quantity is greater than frequency of notes count: \nlengths count:   %ld\nfrequency count: %d\n\n", strlen(lngths),k);
	}
	else if (strlen(lngths) < k)
	{
		printf("WARNING: Lengths of note quantity is less than frequency of notes count: \nlengths count:   %ld\nfrequency count: %d\n\n", strlen(lngths), k);
	}
	
	do {
		//printf("start for: %d\n", j);
		if (startNote && isStartLate) {
			j = startNote - 2;
			printf("Starting Late at Note %d\n", j);
			goto END_WRITE_LOOP;
		}
		// write loop needs minimal timing
		for (j = 0; (j < strlen(lngths)) && (j < k); j++)
		{
			if (endNote && j >= endNote - 1) {
				printf("Ending Early at Note %d\n", j);
				break;
			}

			// Open connection to the sound card
			dsp_open("/dev/dsp", O_WRONLY, AFMT_S16_NE, 1, SAMP_RATE);

			//printf("size of single length: %ld\n", sizeof(lngths));

			//printf("lngth str: %s\n", lngths);
			//printf("0: %s\n", lngths);
			//printf("1: %d\n", lngths[j]);
			switch (lngths[j])
			{
			case '1':
				signal_length = tempoControl;
				break;
			case '2':
				signal_length = (tempoControl * 3) / 4;
				break;
			case '3':
				signal_length = tempoControl / 2;
				break;
			case '4':
				signal_length = tempoControl / 4;
				break;
			case '5':
				signal_length = tempoControl / 8;
				break;
			case '6':
				signal_length = tempoControl / 16;
				break;
			case '7':
				signal_length = tempoControl / 32;
				break;
			case ' ':
				break;
			default:
				printf("note %d: %c\n", j, lngths[j]);
				fprintf(stderr, "Unrecognized note length: %d please re-type length\n", j + 1);
				exit(1);
				break;
			}

			//printf("probe 1 for: %d\n", j);
			r = freqTranslator(&poctave, &pnote, notes[j]);
			if (r) {
				printf("\nfaulty note\n");
				break;
			}
			printf("j = %d:  \t octave: %d \t", j, poctave);
			printf("note: %d \t", pnote);
			printf("notes[j]: %c%c \t", notes[j][0], notes[j][1]);
			printf("length: %c \n", lngths[j]);

			//printf("probe 2 for: %d\n", j);
			notefreq = freq_table[poctave][pnote];



			// Create test signal

			// MULTINOTE Change here
			for (i = 0; i < signal_length; i++)
			{
				signal[i] = 32767 / 2.0*sin((2.0*PI*notefreq*i) / SAMP_RATE);
			}


			//printf("probe 3 for: %d\n", j);

			// Write the test signal
			dsp_write(signal, sizeof(short), signal_length);

			//May be necessary to flush the buffers
			dsp_sync();

			// Close connection to the sound card
			dsp_close();
			//printf("end for: %d\n", j);

		END_WRITE_LOOP:
			;
		}
	} while (isLoopSong);

	if (isManualEntry) {
		goto MANUAL_TONE_ENTRY;
	}

	//Successful exit
	exit(0);
}
