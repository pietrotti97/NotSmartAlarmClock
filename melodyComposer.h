#define MUSIC_NOTE_C    1
#define MUSIC_NOTE_Cd   2
#define MUSIC_NOTE_D    3
#define MUSIC_NOTE_Dd   4
#define MUSIC_NOTE_E    5
#define MUSIC_NOTE_F    6
#define MUSIC_NOTE_Fd   7
#define MUSIC_NOTE_G    8
#define MUSIC_NOTE_Gd   9
#define MUSIC_NOTE_A    10
#define MUSIC_NOTE_Ad   11
#define MUSIC_NOTE_B    12
#define MUSIC_NONE_NONE 0

#define MUSIC_NOTES_NOF 12
#define MUSIC_OCTAVES_NOF 4

#define MUSIC_DB_NOTES_MAX  50
#define MUSIC_BPM_MIN       40  // xx ms
#define MUSIC_BPM_MAX_MS    225 // xx ms

static float notesMatrix[MUSIC_NOTES_NOF][MUSIC_OCTAVES_NOF] = {
  {130.81f, 	261.63f, 	523.25f, 	1046.50f},  // C
  {138.59f, 	277.18f, 	554.37f, 	1108.73f},  // C# 
  {146.83f, 	293.66f,  587.33f, 	1174.66f},  // D
  {155.56f, 	311.13f, 	622.25f, 	1244.51f},  // D#
  {164.81f, 	329.63f, 	659.25f, 	1318.51f},  // E
  {174.61f, 	349.23f, 	698.46f, 	1396.91f},  // F
  {185.00f, 	369.99f, 	739.99f, 	1479.98f},  // F#
  {196.00f, 	392.00f, 	783.99f, 	1567.98f},  // G
  {207.65f, 	415.30f, 	830.61f, 	1661.22f},  // G#
  {220.00f, 	440.00f, 	880.00f,  1760.00f},	// A
  {233.08f, 	466.16f, 	932.33f, 	1864.66f},  // A#
  {246.94f, 	493.88f, 	987.77f, 	1975.53f}   // B
};

typedef struct {
  struct {
    uint8_t note;
    uint8_t octave;
    uint8_t tempo;
  }music[MUSIC_DB_NOTES_MAX];
  uint16_t bpm;
  uint8_t slotsUsed;
} melodyCustom_s;
