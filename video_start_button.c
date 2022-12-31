
// edit usb_desc.h to set VENDOR_ID to 16C1

// Linux: sudo echo 'SUBSYSTEMS=="usb", ATTRS{idVendor}=="16c1", ATTRS{idProduct}=="0486", MODE:="0666"' > /etc/udev/rules.d/00-video-start-button.rules

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include "hid.h"

#if defined(OS_LINUX)
#define CMD1 "ffmpeg -y -hide_banner -f x11grab -video_size 3840x2160 -framerate 59.94 -i :1 -c:v hevc_nvenc -r 59.94 -rc constqp -qp 17 -preset medium -b:v 0 -tag:v hvc1 /tmp/desktop.constqp.mp4"
#define CMD2 "ffmpeg -y -hide_banner -f v4l2 -video_size 800x600 -framerate 59.94 -i /dev/video0 -c:v libx264 -r 59.94 -crf 22 /tmp/scope.vbr.mp4"
#define CMD3 "ffmpeg -y -hide_banner -f alsa -acodec pcm_s32le -channels 4 -sample_rate 44100 -itsoffset -0.203 -i hw:CARD=M4 -crf 0 /tmp/sound.mp4"
#define CMD4 ""
#define HOME "/home/paul"
#define FFMPEG "/usr/bin/ffmpeg"
#define POST1 "/usr/bin/scp /tmp/desktop.constqp.mp4 /tmp/scope.vbr.mp4 /tmp/sound.mp4 studio:"
#define POST2 "/usr/bin/ssh studio open multicam.fcpxml"

#elif defined(OS_MACOSX)
#define CMD1 "ffmpeg -y -hide_banner -f avfoundation -framerate 59.94 -i 0:none -c:v hevc_videotoolbox -r 59.94 -b:v 5M -realtime true -tag:v hvc1 ~/camera1.cbr.mov"
#define CMD2 "ffmpeg -y -hide_banner -f avfoundation -framerate 59.94 -i 1:none -c:v hevc_videotoolbox -r 59.94 -b:v 5M -realtime true -tag:v hvc1 ~/camera2.cbr.mov"
#define CMD3 "ffmpeg -y -hide_banner -f avfoundation -framerate 59.94 -i 2:none -c:v hevc_videotoolbox -r 59.94 -b:v 5M -realtime true -tag:v hvc1 ~/camera3.cbr.mov"
#define CMD4 "ffmpeg -y -hide_banner -f avfoundation -framerate 59.94 -i 3:none -c:v hevc_videotoolbox -r 59.94 -b:v 5M -realtime true -tag:v hvc1 ~/camera4.cbr.mov"
#define HOME "/Users/paul"
#define FFMPEG "/opt/homebrew/bin/ffmpeg"
#define FCPXML "/Users/paul/multicam.fcpxml"

#endif

const char *cmd1arg[100], *cmd2arg[100], *cmd3arg[100], *cmd4arg[100], *postcmd[100];
int pid1=0, pid2=0, pid3=0, pid4=0;


void hexdump(const void *p, int num);
const char *timecode_now(void);
void arglist(const char *commandline, const char *args[], const char *tc);
pid_t execute(const char **commandline);
int xmlupdate(void);


void leds_update(void)
{
	char buf[64];
	memset(buf, 0, sizeof(buf));
	int bits = 0x80; // green LED on as long as we keep transmitting
	if (pid1 > 0) bits |= 0x01;
	if (pid2 > 0) bits |= 0x02; // red LEDs indicate if record process is running
	if (pid3 > 0) bits |= 0x04;
	if (pid4 > 0) bits |= 0x08;
	buf[0] = bits;
	rawhid_send(0, buf, 64, 100);
}


void start(void)
{
	const char *tc = timecode_now();
	arglist(CMD1, cmd1arg, tc);
	arglist(CMD2, cmd2arg, tc);
	arglist(CMD3, cmd3arg, tc);
	arglist(CMD4, cmd4arg, tc);
	//for (const char **p=cmd1arg; *p; p++) printf("cmd1 = \"%s\"\n", *p);
	//for (const char **p=cmd2arg; *p; p++) printf("cmd2 = \"%s\"\n", *p);
	pid1 = execute(cmd1arg);
	pid2 = execute(cmd2arg);
	pid3 = execute(cmd3arg);
	pid4 = execute(cmd4arg);
}

void stop(void)
{
	if (pid1 > 0) kill(pid1, SIGTERM);
	if (pid2 > 0) kill(pid2, SIGTERM);
	if (pid3 > 0) kill(pid3, SIGTERM);
	if (pid4 > 0) kill(pid4, SIGTERM);
	int status;
	if (pid1 > 0) waitpid(pid1, &status, 0);
	if (pid2 > 0) waitpid(pid2, &status, 0);
	if (pid3 > 0) waitpid(pid3, &status, 0);
	if (pid4 > 0) waitpid(pid4, &status, 0);
	pid1 = 0;
	pid2 = 0;
	pid3 = 0;
	pid4 = 0;
}

void poll_rawhid(void)
{
	char buf[64];

	while (1) {
		// check if any Raw HID packet has arrived
		int num = rawhid_recv(0, buf, 64, 500);
		if (num < 0) {
			printf("\nerror reading, device went offline\n");
			rawhid_close(0);
			printf("after close\n");
			break;
		}
		if (num == 0) {
			//printf("rawhid read timeout, update LEDs\n");
			if (pid1 > 0 && kill(pid1, 0) == -1) pid1 = 0;
			if (pid2 > 0 && kill(pid2, 0) == -1) pid2 = 0;
			if (pid3 > 0 && kill(pid3, 0) == -1) pid3 = 0;
			if (pid4 > 0 && kill(pid4, 0) == -1) pid4 = 0;
			leds_update();
		}
		if (num > 0) {
			printf("\nrecv %d bytes:\n", num);
			hexdump(buf, num);
			if (memcmp(buf, "Go Button Press", 16) == 0) {
				printf("Go button\n");
				if (pid1 == 0 && pid2 == 0 && pid3 == 0 && pid4 == 0) {
					// green button starts recording
					start();
				} else {
					// if already running, stop and import result to Final Cut
					xmlupdate();
					stop();
					#ifdef POST1
					arglist(POST1, postcmd, NULL);
					execute(postcmd);
					#endif
					#ifdef POST2
					arglist(POST2, postcmd, NULL);
					execute(postcmd);
					#endif
				}
			}
			if (memcmp(buf, "Stop Button Press", 18) == 0) {
				// red button stops and does not import anything
				printf("Stop button\n");
				stop();
			}
			leds_update();
		}
	}
}


int main()
{
	memset(cmd1arg, 0, sizeof(cmd1arg));
	memset(cmd2arg, 0, sizeof(cmd2arg));
	memset(cmd3arg, 0, sizeof(cmd3arg));
	memset(cmd4arg, 0, sizeof(cmd4arg));
	memset(postcmd, 0, sizeof(postcmd));

	while (1) {
		// edit usb_desc.h to set VENDOR_ID to 16C1
		// Arduino-based example is 16C1:0486:FFAB:0200
		printf("before open\n");
		int r = rawhid_open(1, 0x16C1, 0x0486, 0xFFAB, 0x0200);
		if (r > 0) {
			printf("found rawhid device\n");
			char buf[64];
			while (rawhid_recv(0, buf, 64, 10)) {
				// discard any previously buffered data
			}
			poll_rawhid();
		}
		sleep(1);
	}

}


void hexdump(const void *p, int num)
{
	const unsigned char *b = p;
	const unsigned char *end = b + num;;
	unsigned int count = 0;
	while (b < end) {
		printf("%02X ", *b++);
		if ((++count & 15) == 0) printf("\n");
	}
	if ((count & 15) != 0) printf("\n");
}



const char *timecode_now(void)  // from timecode_now.c
{
	struct timeval tv;
	struct tm t;
	const int fps = 60;
	static char buf[64];

	// get the actual time
	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &t);

	// convert actual to number of video frames at 60 Hz
	int total_frames = (t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec) * fps
		+ (int)tv.tv_usec * fps / 1000000;
	//printf("total frames = %d\n", total_frames);

	// adjust by the algorithm ffmpeg uses
	int tmin = t.tm_hour * 60 + t.tm_min;
	//total_frames -= ((fps == 30) ? 2 : 4) * (tmin - tmin/10); // newer ffmpeg?
	total_frames -= 2 * (tmin - tmin/10); // ffmpeg 3.4.8-0ubuntu0.2
	//printf("adjusted frames = %d\n", total_frames);

	// convert number of video frames back to time
	int frame = total_frames % fps;
	int total_sec = total_frames / fps;
	int total_min = total_sec / 60;
	int hours = total_min / 60;
	int minutes = total_min - hours * 60;
	int seconds = total_sec - (minutes * 60 + hours * 3600);

	snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%02d", hours, minutes, seconds, frame);
	return buf;
}

void arglist(const char *commandline, const char *args[], const char *tc)
{
	int i, argcount = 0;
	const char *p = commandline;

	for (i=0; i < 100; i++) {
		if (args[i] == NULL) break;
		free((void *)args[i]);
		args[i] = NULL;
	}
	while (*p) {
		while (isspace(*p)) p++;
		const char *arg = p;
		while (*p && !isspace(*p)) p++;
		int len = p - arg;
		if (*arg && len > 0) {
			char *s = NULL;
			if (argcount == 0 && len == 6 && strncmp(arg, "ffmpeg", 6) == 0) {
				s = strdup(FFMPEG);
			} else if (len == 14 && strncmp(arg, "`timecode_now`", 14) == 0) {
				s = strdup(tc);
			} else if (len > 1 && arg[0] == '~' && arg[1] == '/') {
				s = malloc(len + strlen(HOME));
				if (s) {
					strcpy(s, HOME);
					strncpy(s + strlen(HOME), arg + 1, len - 1);
					s[strlen(HOME) + len - 1] = 0;
				}
			} else {
				s = strndup(arg, len);
			}
			if (s == NULL) {
				fprintf(stderr, "ERROR allocating memory\n");
				for (i=0; i < argcount; i++) {
					free((void *)args[i]);
					args[i] = NULL;
				}
				argcount = 0;
				break;
			}
			//printf("arg: len=%d, s=\"%s\", \"%s\"\n", len, s, arg);
			args[argcount++] = s;
		}
	}
	args[argcount] = NULL;
	//for (i=0; i < argcount; i++) printf("args[%d] = \"%s\"\n", i, args[i]);
}

pid_t execute(const char **commandline)
{
	if (*commandline == NULL) return 0;
	pid_t pid = fork();
	if (pid == -1) {
		fprintf(stderr, "ERROR, can't create process with fork()\n");
		return 0;
	}
	if (pid == 0) {
		// new child process
		const char *cmd = commandline[0];
		execv(cmd, (char **)commandline);
		exit(1);
	}
	return pid;
}

// file all "clip_##" and increment them
int xmlupdate(void)
{
#ifdef FCPXML
	printf("xmlupdate %s\n", FCPXML);
	// get size of XML file
	struct stat s;
	int r = stat(FCPXML, &s);
	if (r < 0) return 0;
	if (s.st_size <= 0 || s.st_size > 500000) return 0;
	// read entire file into a buffer
	int fd = open(FCPXML, O_RDONLY);
	if (fd < 0) return 0;
	char *buf = malloc(s.st_size + 1);
	if (!buf) {
		close(fd);
		return 0;
	}
	if (read(fd, buf, s.st_size) != s.st_size) {
		free(buf);
		close(fd);
		return 0;
	}
	buf[s.st_size] = 0;
	//printf("read %s, %lu bytes\n", FCPXML, (long)(s.st_size));
	close(fd);
	// now overwrite the file
	FILE *fp = fopen(FCPXML, "w");
	if (!fp) return 0;
	char *p = buf;
	while (*p) {
		char *clip = strstr(p, "clip_");
		if (clip == NULL) {
			fprintf(fp, "%s", p);
			//printf("output %lu bytes, end of file\n", strlen(p));
			break;
		}
		printf("found clip_ string at offset %u\n", (int)(clip - buf));
		if (clip > p) {
			*clip = 0;
			fprintf(fp, "%s", p);
			//printf("output %lu bytes\n", strlen(p));
		}
		if (isdigit(clip[5])) {
			p = clip + 5;
			unsigned int n = 0;
			while (isdigit(*p)) {
				n = n * 10 + *p++ - '0';
			}
			fprintf(fp, "clip_%u", n + 1);
			//printf("clip_%u\n", n + 1);
		} else {
			fprintf(fp, "clip_");
			p += 5;
		}
	}
	fclose(fp);
#endif
	return 1;
}


