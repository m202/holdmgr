/*
	queue_helper: SUID-root CGI script helper to manage postfix queue
	Don't forget to chmod +s me and put the web server's UID in the queue_helper.id file
*/

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define SOURCE_PATH "/var/spool/postfix/hold/"
#define DEST_PATH "/var/spool/postfix/hold/.scanned/"
#define MAX_QUEUE_ID_LENGTH 10
#define POSTSUPER "/usr/sbin/postsuper"
#define POSTQUEUE "/usr/sbin/postqueue"
#define ID_FILE "queue_helper.id"

int main(int argc, char **argv) {

	int i;
	int ret;

	int expected_uid = -1;
	typedef enum { NONE, DELETE, PASS } mode_t;
	mode_t mode = NONE;

	/* Process arguments */
	if(argc != 3) {
		printf("Usage: %s {-d|-p} queue_id\n", argv[0]);
		return 255;
	}

	if(!strcmp(argv[1], "-d")) mode = DELETE;
	else if(!strcmp(argv[1], "-p")) mode = PASS;
	else {
		printf("Usage: %s {-d|-p} queue_id\n", argv[0]);
		return 255;
	}

	/* Check caller's UID  */
	FILE *uid_file = fopen(ID_FILE, "r");
	if(uid_file == NULL) {
		printf("Cannot open UID file\n");
		return 255;
	}

	if(fscanf(uid_file, "%i", &expected_uid) != 1) {
		printf("Error reading UID file\n");
		fclose(uid_file);
		return 255;
	}
	fclose(uid_file);

	if(getuid() != expected_uid) {
		printf("This program must be run by UID %d\n", expected_uid);
		return 255;
	}

	/* Build file strings */
	size_t source_path_len = strlen(DEST_PATH);
	char *source = (char*) malloc(source_path_len + MAX_QUEUE_ID_LENGTH + 1);
	memset(source, 0, source_path_len + MAX_QUEUE_ID_LENGTH + 1);
	strncpy(source, DEST_PATH, source_path_len);
	strncpy(source + source_path_len, argv[2], MAX_QUEUE_ID_LENGTH);

	size_t dest_path_len = strlen(SOURCE_PATH);
	char *dest = (char*) malloc(dest_path_len + MAX_QUEUE_ID_LENGTH + 1);
	memset(dest, 0, dest_path_len + MAX_QUEUE_ID_LENGTH + 1);
	strncpy(dest, SOURCE_PATH, dest_path_len);
	strncpy(dest + dest_path_len, argv[2], MAX_QUEUE_ID_LENGTH);

	/* Set all UID and GID values to zero (postsuper requires this) */
	setresuid(0, 0, 0);
	setresgid(0, 0, 0);

	/* Check for access to queue file */
	if(!access(source, F_OK)) {

		/* Move file back into hold queue */
		if(rename(source, dest)) {
			perror("rename");
			free(source);
			free(dest);
			return 1;
		}

		/* Call postsuper to delete or release message */
		pid_t p = fork();
		if(p == 0) {
			if(mode == DELETE) execl(POSTSUPER, "postsuper", "-d", argv[2], (char*) NULL);
			else if(mode == PASS) execl(POSTSUPER, "postsuper", "-H", argv[2], (char*) NULL);
		} else {
			waitpid(p);
		}

		/* Call postqueue to flush queue */
		p = fork();
		if(p == 0) {
			execl(POSTQUEUE, "postqueue", "-f", (char*) NULL);
		} else {
			waitpid(p);
		}

		if(mode == DELETE) printf("%s deleted\n", argv[2]);
		else if(mode == PASS) printf("%s passed\n", argv[2]);
	} else {
		perror("access");
		printf("Could not access %s\n", argv[2]);
		free(source);
		free(dest);
		return 1;
	}

	free(source);
	free(dest);
	return 0;
}

