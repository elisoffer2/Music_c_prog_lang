//**/** This Is a music synthesizer application used to digitize songs in 8 bit form */

/* lengths of notes:
 * double whole notes require two whole notes
 * 1 - Whole note
 * 2 - three quarter note
 * 3 - Half note
 * 4 - quarter note
 * 5 - 8th note
 * 6 - 16th note
 * 7 - 32nd note
 * 8 - 64th note
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

/* Tempo / BPM conversion to TEST_LEN
 * 80000 --> 24 BPM (maximum)
 * 32000 --> 60 BPM
 * 9600  --> 200 BPM (no lower limit but probably fastest it will go)
 * y = 60*(32000/x) where x is desired BPM and y is the tempocontrol variable  
 */

/* Priotized Product Backlog*/
// --> sprint this (playing poker)
// refactor code
// make one loop instead of init run and then loop
// more functions condense code
// rewrite confusing logic
// music.h for functions and #defines?
// wave shaping and tone options
// key pressing then persistance

// cut out for a later time //
// 17. shuffle play
// 16. song bank / playlist
// 1. remove breaks in notes by continuous phase notes
// output file for .wav for playback save
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
#define TEST_LEN SAMP_RATE*10 //--> max tempo for primary allocation

// maximum size for freqs and lngths (trade off between note count and memory used)
#define BUFF_SIZE 65536

#define MAX_SIMULT_NOTES 20 //--> 40 = duet 10 fingers
#define SAME_NOTE_PAUSE_FACTOR 0.008 // 1/128 of signal

// maximum file size to read in from a song text file
#define FILE_SIZE 65536

// count of bytes taken from a file when copying
#define BUFFERSIZE 1024

// PI used for sine wave generation
#define PI (3.14159265)

// more printouts
#define VERBOSE_SETTING 1

// variables for getLine: https://stackoverflow.com/questions/4023895/how-do-i-read-a-string-entered-by-the-user-in-c
#define OK       0
#define NO_INPUT 1
#define TOO_LONG 2

static int getLine(char *prmpt, char * buff, size_t sz) {
	int ch, extra;

	// Get line with buffer overrun protection.
	if (prmpt != NULL) {
		printf("%s", prmpt);
		fflush(stdout);
	}
	if (fgets(buff, sz, stdin) == NULL)
		return NO_INPUT;

	// If it was too long, there'll be no newline. In that case, we flush
	// to end of line so that excess doesn't affect the next call.
	if (buff[strlen(buff) - 1] != '\n') {
		extra = 0;
		while (((ch = getchar()) != '\n') && (ch != EOF))
			extra = 1;
		return (extra == 1) ? TOO_LONG : OK;
	}

	// Otherwise remove newline and give string back to caller.
	buff[strlen(buff) - 1] = '\0';
	return OK;
}

int * keyExtractor(char noteStr[], int * keys) {
	int key_count = 0;
	int i;
	key_count = strlen(noteStr) / 2;

	for (i = 0; i < key_count; i++)
	{
		switch (noteStr[i * 2]) {
			case 'C':
			case 'c':
				keys[i] = 0;
				break;
			case 'X':
			case 'x':
				keys[i] = 1;
				break;
			case 'D':
			case 'd':
				keys[i] = 2;
				break;
			case 'Y':
			case 'y':
				keys[i] = 3;
				break;
			case 'E':
			case 'e':
				keys[i] = 4;
				break;
			case 'F':
			case 'f':
				keys[i] = 5;
				break;
			case 'Z':
			case 'z':
				keys[i] = 6;
				break;
			case 'G':
			case 'g':
				keys[i] = 7;
				break;
			case 'V':
			case 'v':
				keys[i] = 8;
				break;
			case 'A':
			case 'a':
				keys[i] = 9;
				break;
			case 'W':
			case 'w':
				keys[i] = 10;
				break;
			case 'B':
			case 'b':
				keys[i] = 11;
				break;
			case 'P':
			case 'p':
				keys[i] = 12;
				break;
			case ' ':
				printf("Found space: Error highest note substitute");
				keys[i] = 11;
				break;
			default:
				printf("Note not recognized. substituting with C1");
				keys[i] = 0;
				break;
		}
	}
	return keys;
}

int * octaveExtractor(char noteStr[], int * octaves) {
	
	int key_count = 0;
	int i;
	key_count = strlen(noteStr) / 2;

	for (i = 0; i < key_count; i++)
	{
		octaves[i] = (int)noteStr[(i * 2) + 1] - (int)'1';
		if (octaves[i] > 8 || octaves[i] < 0) {
			printf("OCTAVE OUT of Range! puting in 0");
			octaves[i] = 0;
		}
	}
	
	return octaves;
}

int tempoTranslator(int traditionTempo) {
	int programTempo = 0;
	if (traditionTempo != 0)
		programTempo = 60 * (SAMP_RATE*4 / traditionTempo);
	else {
		printf("ERROR: Tempo was input as zero, setting to 60");
		traditionTempo = 60;
		programTempo = 60 * (SAMP_RATE*4 / traditionTempo);
	}

	return programTempo;
}

int strSpaceCount(char * str) {
	int i;
	int spaces = 0;
	for (i = 0; str[i] != '\0'; i++)
	{
		if (str[i] == ' ')
		{
			spaces++;
		}
	}
	return spaces;
}

char * removeNewLines(char * str) {
	for (int i = 0; i < strlen(str); i++) {
		if (str[i] == '\n') {
			printf("NEW LINE REMOVED\n");
			str[i] = '\0';
			break;
		}
	}
	return str;
}

// Missing Error checking on inputs and int return regarding error for redo
int manualSongEntry(char ** lngths,char ** freqs, char ** tempoStr) {
	int rc;
	char line[BUFF_SIZE];

	/* Manually entering lengths and tones*/
	rc = getLine("Enter Tempo> ", line, sizeof(line));
	if (rc == NO_INPUT) {
		// Extra NL since my system doesn't output that on EOF.
		printf("\nNo input\n");
		return 1;
	}
	if (rc == TOO_LONG) {
		printf("Input too long [%s]\n", line);
		return 1;
	}
	printf("OK [%s]\n", line);
	*tempoStr = (char *)calloc(strlen(line),sizeof(char));
	strcpy(*tempoStr, line);

	getLine("Enter lngths> ", line, sizeof(line));
	if (rc == NO_INPUT) {
		// Extra NL since my system doesn't output that on EOF.
		printf("\nNo input\n");
		return 1;
	}
	if (rc == TOO_LONG) {
		printf("Input too long [%s]\n", line);
		return 1;
	}
	printf("OK [%s]\n", line);
	*lngths = (char *)calloc(strlen(line),sizeof(char));
	strcpy(*lngths, line);

	getLine("Enter freqs> ", line, sizeof(line));
	if (rc == NO_INPUT) {
		// Extra NL since my system doesn't output that on EOF.
		printf("\nNo input\n");
		return 1;
	}
	if (rc == TOO_LONG) {
		printf("Input too long [%s]\n", line);
		return 1;
	}
	printf("OK [%s]\n", line);
	*freqs = (char *)calloc(strlen(line), sizeof(char));
	strcpy(*freqs, line);
	
	return 0;

}

int fileSongEntry(char ** lngths, char ** freqs, char ** tempoStr, char * fileName) {

	int i;
	char musicFileStr[BUFF_SIZE*3];
	char * pnoteStr;
	FILE * fp;
	char buffer[BUFFERSIZE];

	strcpy(musicFileStr, "");
	fp = fopen(fileName, "r"); //so if there was 3 arguements open the file you want to display on stdout
	if ((fp) == NULL)          //if the filepointer is \0 then that file doesn't exist close program
	{
		fprintf(stderr, "File does not exist!\n");
		return (1);
	}

	fseek(fp, 0, SEEK_SET);
	do {
		memset(buffer, 0, strlen(buffer));			// clear the buffer so there is no excess when reading the last bit of the file
		i = fread(buffer, 1, BUFFERSIZE, fp);		// read 512 bytes of the file so long and its not the end of file there will be exactly 512 bytes 
		strcat(musicFileStr, buffer);
	} while (i == BUFFERSIZE);						//i is updating every while loop by re-reading the file pointed to by *fp when i != Buffersize we reached the end of the file
	if (VERBOSE_SETTING) {
		printf("\n\n"); // space out file read output to stdout:
	}
	if (VERBOSE_SETTING) {
		printf("musicFileStr After full Read: \n%s\n\n", musicFileStr);
	}

	pnoteStr = strtok(musicFileStr, "\n");
	*tempoStr = (char *)calloc(strlen(pnoteStr), sizeof(char));
	strncpy(*tempoStr, pnoteStr, strlen(pnoteStr)-1);
	(*tempoStr)[strlen(pnoteStr)] = '\0';


	pnoteStr = strtok(NULL, "\n");
	*freqs = (char *)calloc(strlen(pnoteStr), sizeof(char));
	strncpy(*freqs, pnoteStr, strlen(pnoteStr) - 1);
	(*freqs)[strlen(pnoteStr)] = '\0';

	pnoteStr = strtok(NULL, " ,.-");
	*lngths = (char *)calloc(strlen(pnoteStr), sizeof(char));
	strcpy(*lngths, pnoteStr);

	strcpy(pnoteStr, "");

	return 0;
}

char ** noteExtractor(int * pnote_count, char * freqs, char ** notes) {

	char * pnoteStr;
	int note_count_2 = *pnote_count;

	*pnote_count = 0;
	pnoteStr = strtok(freqs, " ,.-");
	if (VERBOSE_SETTING) {
		printf("\n");
	}
	while (pnoteStr != NULL)
	{
		if (VERBOSE_SETTING) {
			printf("note_count = %d: %s, %ld\n", *pnote_count, pnoteStr, strlen(pnoteStr));
		}
		strcpy(notes[*pnote_count], pnoteStr);
		pnoteStr = strtok(NULL, " ,.-");
		(*pnote_count)++;
	}

	if (*pnote_count != note_count_2) {
		printf("\t\tNOTES NOT MATCHING UP\n");
	}
	
	return notes;
}

int noteTimeCheck(int lcount, int fcount) {
	if (lcount > fcount)
	{
		printf("WARNING: Lengths of note quantity is greater than frequency of notes count: \nlengths count:   %d\nfrequency count: %d\n\n", lcount, fcount);
		return 0;
	}
	else if (lcount < fcount)
	{
		printf("WARNING: Lengths of note quantity is less than frequency of notes count: \nlengths count:   %d\nfrequency count: %d\n\n", lcount, fcount);
		return 2;
	}
	return 0;
}

unsigned long long generateSignal (unsigned int tempoControl, char * lngths, char ** notes, int freq_count, int startNote, \
	                               int endNote, unsigned short * song_signal, unsigned long long song_counter) {
	int i = 0;
	int j = 0;
	int k = 0;;
	
	unsigned long signal_length = 0;
	unsigned short ** note_signal;
	unsigned short comb_note_signal[tempoControl];
	int * pnote;
	int * poctave;
	unsigned short key_count;
	int * notefreq;
	int isSameNote = 0;

	note_signal = (unsigned short **)malloc(MAX_SIMULT_NOTES * sizeof(unsigned short *));
	for (i = 0; i < MAX_SIMULT_NOTES; i++) {
		note_signal[i] = (unsigned short *)malloc(tempoControl * sizeof(unsigned short));
	}

	// Table of piano keys frequencies by octave and key
	//						  C		C#/Db D	    D#/Eb E		F	  F#/Gb G	  G#/Ab	A	  A#/Bb B	  pause
	int freq_table[7][13] = {{33,   35,   37,   39,   41,   44,   46,   49,   52,   55,   58,   62,   0},	// Octave 1
							 {65,   69,   73,   78,   82,   87,   92,   98,   104,  110,  117,  123,  0},	// Octave 2
							 {131,  139,  147,  156,  165,  175,  185,  196,  208,  220,  233,  247,  0},	// Octave 3
							 {262,  277,  294,  311,  330,  349,  370,  392,  415,  440,  466,  494,  0},	// Octave 4
							 {523,  554,  587,  622,  659,  698,  740,  784,  831,  880,  932,  988,  0},	// Octave 5
							 {1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976, 0},	// Octave 6
							 {2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951, 0}};	// Octave 7
	
	if (startNote) {
		j = startNote;
		if (VERBOSE_SETTING) {
			printf("Starting Late at Note %d\n", j);
		}
	}
	else {
		j = 0;
	}
	while((j < strlen(lngths)) && (j < freq_count))
	{
		if (endNote && j >= endNote) {
			if (VERBOSE_SETTING) {
				printf("Ending Early at Note %d\n", j);
			}
			break;
		}

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
			case '8':
				signal_length = tempoControl / 64;
				break;
			case ' ':
				break;
		}

		//printf("probe 1 for: %d\n", j);
		key_count = strlen(notes[j])/2;
		pnote = (int*)calloc(key_count, sizeof(int));
		poctave = (int*)calloc(key_count, sizeof(int));
		notefreq = (int*)calloc(key_count, sizeof(int));

		pnote = keyExtractor(notes[j],pnote);
		poctave = octaveExtractor(notes[j], poctave);
		if (VERBOSE_SETTING) {
			printf("j = %d:  \t", j);
			printf("length: %c \t", lngths[j]);
			printf("notes[j]: %s \n", notes[j]);
		}

		for (k = 0; k < key_count; k++) {
			notefreq[k] = freq_table[poctave[k]][pnote[k]];
			if (VERBOSE_SETTING) {
				printf("notefreq[%d]: %d\t", k, notefreq[k]);
			}
		}
		if (VERBOSE_SETTING) {
			printf("\n");
		}
		// Create test signal
		for (i = 0; i < signal_length; i++)
		{
			comb_note_signal[i] = 0;
			if ((j != strlen(lngths)-1) && (j != freq_count-1)) {
				isSameNote = !strcmp(notes[j], notes[j + 1]);
			}
			

			for (k = 0; k < key_count; k++) {
				if (isSameNote && (i < tempoControl*SAME_NOTE_PAUSE_FACTOR)) {
					note_signal[k][i] = 0;
				}
				else {
					note_signal[k][i] = 32767 / key_count * sin((2.0*PI*notefreq[k] * i) / SAMP_RATE);
				}
				comb_note_signal[i] += note_signal[k][i];
			}
			

			song_signal[song_counter] = comb_note_signal[i];
			song_counter++;
		}

		free(notefreq);
		free(pnote);
		free(poctave);

		j++;
	}
	
	for (i = 0; i < MAX_SIMULT_NOTES; i++) {
		free(note_signal[i]);
	}
	free(note_signal);
	return song_counter;
}

int writeSong(unsigned short * song_signal, unsigned long long song_counter) {
	// Open connection to the sound card
	if (VERBOSE_SETTING) {
		printf("Open Sound Card\n");
	}
	dsp_open("/dev/dsp", O_WRONLY, AFMT_S16_NE, 1, SAMP_RATE);

	// Write the test signal
	if (VERBOSE_SETTING) {
		printf("Write to Sound Card\n");
	}
	dsp_write(song_signal, sizeof(short), song_counter);

	//May be necessary to flush the buffers
	if (VERBOSE_SETTING) {
		printf("Sync Sound Card\n");
	}
	dsp_sync();

	// Close connection to the sound card
	if (VERBOSE_SETTING) {
		printf("Close Sound Card\n");
	}
	dsp_close();

	return 0;
}

int setUpSong(int argc, char * argv[], char ** tempoStr, char ** lngths, char ** freqs, int * pstartNote, int * pendNote, int * ploopSong) {
	// collecting relevent info: 
	// inputs: argc and argv
	// internal variables: fp, musicFileStr, buffer pnotestr
	// outputs: tempoStr lngths freqs, end note, start note, loopSong
	// if there are less than two arguements (runing music alone) then the user needs to input the song

	if (argc < 2)
	{
		manualSongEntry(lngths, freqs, tempoStr);
	}

	// We are loading in a song with specific settings
	else
	{
		if (fileSongEntry(lngths, freqs, tempoStr, argv[1])) {
			fprintf(stderr, "File Song Entry Failure\n");
			exit(1);
		}

		// start offset
		if (argc > 2)
		{
			//offset is string to long of the byte offset specified by the user. so string of numbers goes to the offset value as a number value integer
			*pstartNote = atoi(argv[2]);
			if (VERBOSE_SETTING) {
				printf("Start Note Init: %d\n", *pstartNote);
			}
		}
		// end offset
		if (argc > 3)
		{
			// and argv[2] is a character array of the integers given which we want to convert to an "integer", but really a long so we can offset that interger value
			*pendNote = atoi(argv[3])+1; // +1 because we want to play the note we end early at and 
			if (VERBOSE_SETTING) {
				printf("End Note Init: %d\n", *pendNote);
			}
		}

		// loop the song
		if (argc > 4)
		{
			*ploopSong = atoi(argv[4]);
		}
	}
	return 0;
}

int main(int argc, char *argv[])
{
	// counters used in the program
	int i;
	int	note_counter;

	// signal processing variables
	char * tempoStr;
	unsigned int tempoControl = TEST_LEN;
	// Maybe use malloc, remove TEST_LEN all together to remove limitation and free
	
	unsigned short * song_signal; // very large variable
	unsigned long long song_counter = 0;

	// frequencies of notes variables
	char ** notes;
	char * freqs;				// note frequencies array

	// length of note variables
	char * lngths;				// note lengths array

	// frequencies and length manipulation variables
	char * pnoteStr;

	// playback settings:
	int startNote = 0;
	int endNote = 0;
	int loopSong = 0;
	
	song_signal = (unsigned short*)calloc(tempoControl*BUFF_SIZE, sizeof(unsigned short));

	setUpSong(argc, argv, &tempoStr, &lngths, &freqs, &startNote, &endNote, &loopSong);

	freqs = removeNewLines(freqs);
	tempoStr = removeNewLines(tempoStr);
	lngths = removeNewLines(lngths);

	if (VERBOSE_SETTING) {
		printf("Tempo:      |%s|\n", tempoStr);
		printf("Freqs:      |%s|\n", freqs);
		printf("Lengths:    |%s|\n", lngths);
		printf("Start Note: %d\n", startNote);
		printf("End Note:   %d\n", endNote);
		printf("Loop Song:  %d\n", loopSong);
	}
	
	// Setting the Tempo control
	tempoControl = tempoTranslator(atoi(tempoStr));

	// make sure the note lengths*notecount*tempo*sizeof(unsigned short) is not overflowing song signal
	if (0) { // MISSING error condition
		fprintf(stderr, "Error: song is too long and taking up too much memory: tempo: %d", tempoControl);
		exit(1);
	}

	// make space for note and extract them
	note_counter = strSpaceCount(freqs) + 1;
	notes = (char **)calloc(note_counter, sizeof(char*));
	for (i = 0; i < note_counter; i++) {
		(notes[i]) = (char*)calloc(MAX_SIMULT_NOTES*2, sizeof(char));
	}
	notes = noteExtractor(&note_counter, freqs, notes);
	noteTimeCheck(strlen(lngths), note_counter);
	
	song_counter = generateSignal(tempoControl, lngths, notes, note_counter, startNote, endNote, song_signal, song_counter);
	
	if (argc > 1) {
		song_signal = realloc(song_signal, sizeof(short)*song_counter);
	}

	if (VERBOSE_SETTING) {
		printf("\nLength of song: %lld \t sizeof(song_signal): %ld\n\n", song_counter, sizeof(song_signal));
	}
	
	if (!loopSong) {
		free(lngths);
		free(freqs);
		free(tempoStr);
		for (i = 0; i < note_counter; i++)
		{
			free(notes[i]);
		}
		free(notes);
	}

	writeSong(song_signal, song_counter);

	// loop for manual entry
	while (argc < 2) {
		
		setUpSong(argc, argv, &tempoStr, &lngths, &freqs, &startNote, &endNote, &loopSong);
		
		// Setting the Tempo control
		tempoControl = tempoTranslator(atoi(tempoStr));

		// make sure the note lengths*notecount*tempo*sizeof(unsigned short) is not overflowing song signal
		if (0) { // MISSING error condition
			fprintf(stderr, "Error: song is too long and taking up too much memory: tempo: %d", tempoControl);
			exit(1);
		}

		// MULTINOTE Change here
		// get notes from freq
		note_counter = 0;
		if (VERBOSE_SETTING) {
			printf("Splitting string |%s| into tokens: \n", freqs);
		}


		pnoteStr = strtok(freqs, " ,.-");
		while (pnoteStr != NULL)
		{
			if (VERBOSE_SETTING) {
				printf("j = %d: %s\n", note_counter, pnoteStr);
			}
			strcpy(notes[note_counter], pnoteStr);
			pnoteStr = strtok(NULL, " ,.-");
			note_counter++;
		}

		if (VERBOSE_SETTING) {
			printf("\nlen: %ld\n", strlen(lngths));
			printf("frq: %d\n\n", note_counter);
		}
		// end get freq

		noteTimeCheck(strlen(lngths), note_counter);
		
		if (VERBOSE_SETTING) {
			printf("Clearing the Song\n");
		}
		memset(song_signal, 0, sizeof(short)*song_counter);
		song_counter = 0;

		song_counter = generateSignal(tempoControl, lngths, notes, note_counter, startNote, endNote, song_signal, song_counter);
		if (VERBOSE_SETTING) {
			printf("Song Generated\n");
		}

		writeSong(song_signal, song_counter);
		free(lngths);
		free(freqs);
		free(tempoStr);
		for (i = 0; i < note_counter; i++)
		{
			free(notes[i]);
		}
		free(notes);
	}

	//loop song replaying
	while (loopSong) {
		printf("Replaying Song from file\n");

		if (VERBOSE_SETTING) {
			printf("Clearing the Song\n");
		}
		memset(song_signal, 0, sizeof(short)*song_counter);
		song_counter = 0;

		song_counter = generateSignal(tempoControl, lngths, notes, note_counter, startNote, endNote, song_signal, song_counter);

		if (VERBOSE_SETTING) {
			printf("Song Generated\n");
		}
		writeSong(song_signal, song_counter);

		if (loopSong == 2) {
			break;
		}

		if (loopSong > 1) {
			loopSong--;
		}
	}

	free(song_signal);

	//Successful exit
	exit(0);
}
