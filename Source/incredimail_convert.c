//***********************************************************************************************
//     The contents of this file are subject to the Mozilla Public License
//     Version 1.1 (the "License"); you may not use this file except in
//     compliance with the License. You may obtain a copy of the License at
//     http://www.mozilla.org/MPL/
//
//     Software distributed under the License is distributed on an "AS IS"
//     basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
//     License for the specific language governing rights and limitations
//     under the License.
//
//     The Original Code is ReynardWare Incredimail Converter.
//
//     The Initial Developer of the Original Code is David P. Owczarski, created March 20, 2009
//
//     Contributor(s): Christopher Smowton <chris@smowton.net>
//
//************************************************************************************************
#define _CRT_SECURE_NO_WARNINGS  // remove the new mircosoft security warnings

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "sqlite3.h"
#include "increadimail_convert.h"
#include "base64.h"

const char *ATTACHMENT = "----------[%ImFilePath%]----------";

typedef struct {
	char **array;
	size_t used;
	size_t size;
} Array;

void initArray(Array* a, size_t initialSize) {
	a->array = malloc(initialSize * sizeof(char*));
	for (int i = 0; i < initialSize; i++)
		a->array[i] = malloc((512 + 1) * sizeof(char));
	a->used = 0;
	a->size = initialSize;
}

void pushArray(Array* a, char* element) {
	// a->used is the number of used entries, because a->array[a->used++] updates a->used only *after* the array has been accessed.
	// Therefore a->used can go up to a->size 
	if (a->used == a->size) {
		a->size *= 2;
		//a->array = realloc(a->array, a->size * sizeof(int));
		a->array = realloc(a->array, a->size * sizeof(char*));
		for (int i = a->used; i < a->size; i++)
			a->array[i] = malloc((512 + 1) * sizeof(char));
	}

	strcpy(a->array[a->used], element);
	char* arrayValue;
	arrayValue = a->array[a->used];
	a->used++;
}

void freeArray(Array* a) {
	free(a->array);
	a->array = NULL;
	a->used = a->size = 0;
}

void email_count( const char *filename, int *email_total, int *deleted_emails ) {
int dummy = 1;
int i;
int e_count = 0;
int d_count = 0;
HANDLE helping_hand;
char version[5];
unsigned int size = 0;
unsigned int file_size;

   ZeroMemory( &version, sizeof( version ) );
   helping_hand = CreateFile( filename, GENERIC_READ, 0x0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
   
   if( helping_hand ) {
      ReadFile( helping_hand, &version, 0x04, &dummy, NULL );
   }

   SetFilePointer( helping_hand, 0, NULL, FILE_BEGIN );
   do {
      SetFilePointer( helping_hand, 0x0C, NULL, FILE_CURRENT );  // header
      ReadFile( helping_hand, &size, 0x01, &dummy, NULL );
      if( size == 0x02 ) {
        d_count++;
      }
      SetFilePointer( helping_hand, 0x19, NULL, FILE_CURRENT );  // header

      for( i = 0; i < 3; i++ ) {
         ReadFile( helping_hand, &size, 0x04, &dummy, NULL );
         if( !strncmp( version, "V#05", 4 ) ) {
            SetFilePointer( helping_hand, size * 2, NULL, FILE_CURRENT );
         } else {
            SetFilePointer( helping_hand, size, NULL, FILE_CURRENT );
         }
      }

      SetFilePointer( helping_hand, 0x0C, NULL, FILE_CURRENT );
      ReadFile( helping_hand, &size, 0x04, &dummy, NULL );
      if( !strncmp( version, "V#05", 4 ) ) {
         SetFilePointer( helping_hand, size * 2, NULL, FILE_CURRENT );
      } else {
         SetFilePointer( helping_hand, size, NULL, FILE_CURRENT );
      }

      SetFilePointer( helping_hand, 0x06, NULL, FILE_CURRENT );
      ReadFile( helping_hand, &file_size, 0x04, &dummy, NULL );
      SetFilePointer( helping_hand, 0x08, NULL, FILE_CURRENT );
      e_count++;
   } while( dummy );
   CloseHandle( helping_hand );
   e_count--;

   *deleted_emails = d_count;
   *email_total   = e_count;
}


void get_email_offset_and_size( char *filename, unsigned int *file_offset, unsigned int *size, int email_index, int e_count, int *deleted_email ) {
HANDLE helping_hand;
int dummy = 1;
int i, j;
char version[5];
unsigned int file_size, sizer;

   ZeroMemory( &version, sizeof( char ) * 5 );

   helping_hand = CreateFile( filename, GENERIC_READ, 0x00000000, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
   ReadFile( helping_hand, &version, 0x04, &dummy, NULL );
   
   SetFilePointer( helping_hand, 0, NULL, FILE_BEGIN );
   for( i = 0; i < email_index + 1; i++ ) {
      sizer = 0;
      SetFilePointer( helping_hand, 0x0C, NULL, FILE_CURRENT );  // header
      ReadFile( helping_hand, &sizer, 0x01, &dummy, NULL );
      if( sizer == 0x02 ) {
         *deleted_email = 1;
      } else {
         *deleted_email = 0;
      }
      SetFilePointer( helping_hand, 0x19, NULL, FILE_CURRENT );

      for( j = 0; j < 3; j++ ) {
         ReadFile( helping_hand, &sizer, 0x04, &dummy, NULL );
         if( !strncmp( version, "V#05", 4 ) ) {
            SetFilePointer( helping_hand, sizer * 2, NULL, FILE_CURRENT );
         } else {
            SetFilePointer( helping_hand, sizer, NULL, FILE_CURRENT );
         }
      }

      SetFilePointer( helping_hand, 0x0C, NULL, FILE_CURRENT );      
      ReadFile( helping_hand, &sizer, 0x04, &dummy, NULL );
      if( !strncmp( version, "V#05", 4 ) ) {
         SetFilePointer( helping_hand, sizer * 2, NULL, FILE_CURRENT );
      } else {
         SetFilePointer( helping_hand, sizer, NULL, FILE_CURRENT );
      }
      SetFilePointer( helping_hand, 0x06, NULL, FILE_CURRENT );
      file_size = 0;
      ReadFile( helping_hand, &file_size, 0x04, &dummy, NULL );
      *size = file_size;

      // I think this is the file offset
      ReadFile( helping_hand, &sizer, 0x04, &dummy, NULL );
      *file_offset = sizer;
      
      SetFilePointer( helping_hand, 0x04, NULL, FILE_CURRENT );
   }
   CloseHandle( helping_hand );
}


void extract_eml_files( const char *filename_data, char *eml_filename, int offset, unsigned int size ) {
HANDLE helping_hand;
HANDLE writing_hand;
int j, k, dummy;
char extract_data[1024];

      helping_hand = CreateFile( filename_data, GENERIC_READ, 0x0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
      writing_hand = CreateFile( eml_filename, GENERIC_WRITE, 0x0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
      SetFilePointer( helping_hand, offset, NULL, FILE_BEGIN );
      k = size / 1024;
      for( j = 0; j < k; j++ ) {
         ReadFile( helping_hand, &extract_data, 1024, &dummy, NULL );
         WriteFile( writing_hand, &extract_data, 1024, &dummy, NULL );
      }
      k = size % 1024;
      ReadFile( helping_hand, &extract_data, k, &dummy, NULL );
      WriteFile( writing_hand, &extract_data, k, &dummy, NULL );
      CloseHandle( helping_hand );
      CloseHandle( writing_hand );
}

void insert_attachments( char *im_filename, const char *attachments_path, const char *final_email_filename, const char* eml_file_basepath) {

   FILE *inputfile, *outputfile;
   FILE *attachmentfile, *attachmentCopyfile;
   FILE *encode64_input_file;

char string_1[512];
char attachment_name[512];
char attachment_name2[512];
int attachment_length;

char* attachment_extensionPos;
char* attachment_lastExtPos;
char* attachment_lastExtPos2;
char attachment_nameToExport[512];

char temp_path[MAX_CHAR];

int attachment_id;

attachment_id = 0;

// Creating a Map and ensure we don't duplicate attachement, some IM email contains multiple ref (+110372 ref) to the same attachment in my test db
Array attachmentCopied;
initArray(&attachmentCopied, 10);

   inputfile = fopen(im_filename, "rb");
   outputfile = fopen(final_email_filename, "wb");
   
   GetTempPath( sizeof( temp_path ), temp_path );

   if( inputfile && outputfile ) {
      while( fgets(string_1, 512, inputfile) ) {
         // search for the ATTACHMENT string
         if( strncmp( ATTACHMENT,  string_1, 34 ) == 0 ) {
            // fix the attachment string
            attachment_length = (int) strlen(string_1);
            strcpy( attachment_name, attachments_path );
            strcat( attachment_name, "\\" );
            strncat( attachment_name, &string_1[34], attachment_length - 36 );  
			
			//
			// Skip if we already copied it
			//
			int skipFlag = 0;
			if ( attachment_id ) {
				// Look if we already did it, if not, keep it track and move on
				for (int i = 0; i < attachmentCopied.used; i++) {
					if (!strcmp(attachment_name, attachmentCopied.array[i])) {
						skipFlag = 1;
						break;
					}
				}
				if (skipFlag) {
					// Skip this attachment entry and move it on
					fwrite(string_1, 1, strlen(string_1), outputfile);
					continue;
				}
			}
			pushArray(&attachmentCopied, attachment_name);


			//
			// Copy the attachement locally
			//
			// Extension
			
			strcpy(attachment_name2, attachment_name);
			attachment_extensionPos = strtok(attachment_name2, ".");
			while (attachment_extensionPos != NULL)
			{
				attachment_lastExtPos = attachment_extensionPos;
				attachment_extensionPos = strtok(NULL, ".");
			}
			// attachment_lastExtPos
			
			// ++++++++++++++++++++++++++++
			// 
			// Ensure we don't duplicate the attachment, Like "Unnamed Account 1/DANSE SUZIE/email6160 - Re D�cose de la salle.eml"
			//
			// TODO....
			//
			// +++++++++++++++++++++++++++

			// Filename
			attachment_id++;
			char* numberstring[(((sizeof attachment_id) * CHAR_BIT) + 2) / 3 + 2];
			sprintf(numberstring, "%d", attachment_id);
			//c = (char)attachment_id;
			strcpy(attachment_nameToExport, eml_file_basepath);
			strcat(attachment_nameToExport, "-");
			strcat(attachment_nameToExport, numberstring);
			strcat(attachment_nameToExport, ".");
			strcat(attachment_nameToExport, attachment_lastExtPos);

			//strcat(attachment_name, ".");
			//strcat(attachment_name, attachment_lastExtPos2);

			// copy it
			CopyFile(attachment_name, attachment_nameToExport, FALSE);

			//attachmentfile = fopen(attachment_name, "rb");
			//attachmentCopyfile = fopen(attachment_nameToExport, "wb");
			//fwrite( )

            // encode the attachement
			encode64_input_file = fopen(attachment_name, "rb");
			if (encode64_input_file) {
				encode(encode64_input_file, outputfile, 72);
				fclose(encode64_input_file);
			}
         } else {
            fwrite( string_1, 1, strlen(string_1), outputfile );
         }
      }
   }

   freeArray(&attachmentCopied);

   //if (inputfile ) {
	   fclose(inputfile);
   //}
	//if ( outputfile) {
		fclose(outputfile);
	//}

}

enum find_multipart_states {
	FMS_START,
	FMS_CONTENTTYPE,
	FMS_HEADEREND1,
	FMS_HEADEREND2,
	FMS_BOUNDARY,
	FMS_DONE
};

void im_to_eml(char* im_filename, const char* attachments_path, const char* eml_filepath, const char* eml_file_basepath) {

	// Step 1: modify in-place if necessary to correct a Mime content-type that uses imbndry instead of boundary.
	char buf[4096];
	char first_boundary[256];
	first_boundary[0] = '\0';

	FILE* fin = fopen(im_filename, "r");
	enum find_multipart_states state = FMS_START;

	while (fgets(buf, 4096, fin) && first_boundary[0] == '\0') {
		int linelen = strlen(buf);
		if (linelen > 0 && buf[linelen - 1] != '\n')
			fprintf(stderr, "May have missed an important header in %s\r\n", im_filename);
		switch (state) {
		case FMS_START:
			if (strstr(buf, "Content-Type: multipart"))
				state = FMS_CONTENTTYPE;
			// Fall through
		case FMS_CONTENTTYPE:
			if (strstr(buf, "imbndary="))
				state = FMS_HEADEREND1;
			break;
		case FMS_HEADEREND1:
			if (!strcmp(buf, "\n"))
				state = FMS_HEADEREND2;
			break;
		case FMS_HEADEREND2:
			if (!strcmp(buf, "\n"))
				state = FMS_BOUNDARY;
			break;
		case FMS_BOUNDARY:
			if (!strncmp(buf, "--", 2)) {
				const char* lineend = strchr(buf, '\n');
				int copychars = strlen(buf) - 2;
				if (!lineend)
					fprintf(stderr, "MIME boundary without newline?");
				else
					--copychars;
				strncpy_s(first_boundary, 256, buf + 2, copychars);
				state = FMS_DONE;
			}
			break;
		default:
			break;
		}
	}

	if (first_boundary[0] != '\0') {

		int needed = strlen(im_filename) + 10;
		char* fixed_name = malloc(needed);
		if (!fixed_name) {
			fprintf(stderr, "Failed to allocate %d bytes in im_to_eml\n", needed);
			exit(1);
		}
		
		sprintf_s(fixed_name, needed, "%s_fixed", im_filename);

		fixed_name = fixed_name + 

		fseek(fin, 0, SEEK_SET);
		FILE* fout = fopen(fixed_name, "w");
		if (!fout) {
			fprintf(stderr, "Failed to open %s in im_to_eml\n", fixed_name);
			exit(1);
		}

		state = FMS_START;

		while (fgets(buf, 4096, fin)) {
			
			switch (state) {
			case FMS_START:
				if (strstr(buf, "Content-Type: multipart"))
					state = FMS_CONTENTTYPE;
				// Fall through
			case FMS_CONTENTTYPE:
			{
				char* boundary_loc = strstr(buf, "imbndary=");
				if(boundary_loc) {
					int off = boundary_loc - buf;
					sprintf_s(boundary_loc, 4096 - off, "boundary=\"%s\"\n", first_boundary);
					state = FMS_DONE;
				}
				break;
			}
			default:
				break;
			}

			fwrite(buf, 1, strlen(buf), fout);

		}

		fclose(fout);
		fclose(fin);

		DeleteFile(im_filename);
		MoveFile(fixed_name, im_filename);

	}
	else {

		fclose(fin);

	}

	// Step 2: Weave in any attachments that IncrediMail has placed out-of-line:
	insert_attachments(im_filename, attachments_path, eml_filepath, eml_file_basepath);

	// Check if this email has any attachement file in the attachement folder.
	// Copy them into the same place where is the email like: email123-1.docx
	// {emailID}-{attachement seq id}{5 last attachement character}
	// eml_file_basepath
	//std::string source_path;


}

void get_database_version(char *database, char *version ) {
HANDLE helping_hand;
int dummy = 1;
char temp_version[5];

   ZeroMemory( &temp_version, sizeof( temp_version ) );
   helping_hand = CreateFile( database, GENERIC_READ, 0x00000000, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
   ReadFile( helping_hand, &temp_version, 0x04, &dummy, NULL );
   CloseHandle( helping_hand );

   strcpy( version, temp_version );

}

static int testimdb(const char* filename) {

	struct _stat statbuf;
	sqlite3 *db;
	sqlite3_stmt *stmt;
	int ret = 0;

	if (_stat(filename, &statbuf) != 0)
		return 0;

	if (sqlite3_open_v2(filename, &db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK)
		return 0;

	if (sqlite3_prepare_v2(db, "select count(*) from Containers", -1, &stmt, NULL) != SQLITE_OK)
		goto outdb;

	if (sqlite3_step(stmt) != SQLITE_ROW)
		goto outstmt;

	if (sqlite3_column_int(stmt, 0) == 0)
		goto outstmt;

	ret = 1;

outstmt:
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);

outdb:
	sqlite3_close(db);

	return ret;

}

static int endswith_case_insensitive(const char *s, const char *suffix) {
	size_t suffix_len = strlen(suffix);
	size_t s_len = strlen(s);

	return s_len >= suffix_len && !stricmp(s + (s_len - suffix_len), suffix);
}

enum INCREDIMAIL_VERSIONS FindIncredimailVersion(const char *file_or_directory) {
	char temp_path[MAX_CHAR];
	enum INCREDIMAIL_VERSIONS ret = INCREDIMAIL_VERSION_UNKNOWN;
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;

	size_t file_or_directory_len = strlen(file_or_directory);
	const char *file_or_directory_end = file_or_directory + file_or_directory_len;

	// First check if a specific file was given:
	const char *xe_suffix = ".imh";
	const char *im2_mbox_suffix = "\\containers.db";
	const char *im2_maildir_suffix = "\\messageStore.db";
	
	if (endswith_case_insensitive(file_or_directory, xe_suffix))
		return INCREDIMAIL_XE;
	else if (endswith_case_insensitive(file_or_directory, im2_mbox_suffix) && testimdb(file_or_directory))
		return INCREDIMAIL_2;
	else if (endswith_case_insensitive(file_or_directory, im2_maildir_suffix) && testimdb(file_or_directory))
		return INCREDIMAIL_2_MAILDIR;

	// Otherwise try searching a directory:

	// is there an Incredimail XE header file 
	strcpy(temp_path, file_or_directory);
	strcat(temp_path, "\\*.imh");
	hFind = FindFirstFile(temp_path, &FindFileData);
	FindClose(hFind);

	const char *found_files[3];
	size_t n_tests_succeeded = 0;

	if (hFind != INVALID_HANDLE_VALUE) {
		ret = INCREDIMAIL_XE;
		found_files[n_tests_succeeded++] = "* At least one .imh file, as used by Incredimail XE\n";
	}

	// is there an Incredimail 2 database, i.e. containers.db 
	strcpy(temp_path, file_or_directory);
	strcat(temp_path, "\\containers.db");

	if (testimdb(temp_path)) {
		ret = INCREDIMAIL_2;
		found_files[n_tests_succeeded++] = "* A containers.db file, as used by earlier versions of Incredimail 2\n";
	}

	// Is there a messageStore.db, indicating maildir format?
	strcpy(temp_path, file_or_directory);
	strcat(temp_path, "\\messageStore.db");
	if (testimdb(temp_path)) {
		ret = INCREDIMAIL_2_MAILDIR;
		found_files[n_tests_succeeded++] = "* A MessageStore.db file, as used by newer versions of Incredimail 2\n";
	}

	if (n_tests_succeeded > 1) {
		const char *msgHeader = "Multiple Incredimail database types were found:\n";
		const char *msgFooter = "The last one (as the newest format) has been selected for now, but you might want to try selecting an individual database to convert";
		size_t msgLength = strlen(msgHeader) + strlen(msgFooter);
		for (size_t i = 0; i < n_tests_succeeded; ++i) {
			msgLength += strlen(found_files[i]);
		}
		msgLength++; // For final null terminator

		char *msg = (char *)malloc(msgLength);
		msg[0] = '\0';
		if (!msg) {
			MessageBox(global_hwnd, msg, "Failed to allocate memory in FindIncredimailVersion", MB_OK);
			exit(1);
		}

		strcat_s(msg, msgLength, msgHeader);
		for (size_t i = 0; i < n_tests_succeeded; ++i) {
			strcat_s(msg, msgLength, found_files[i]);
		}
		strcat_s(msg, msgLength, msgFooter);

		MessageBox(global_hwnd, msg, "Warning", MB_OK);

		free(msg);
	}

    return ret;
}

void Incredimail_2_Maildir_Email_Count(const char *filename, int *email_total, int *deleted_emails) {

	sqlite3 *db;
	sqlite3_stmt *stmt;

	// Entry with empty location require file split
	//const char* allMailQuery = "select count(*) from Headers where Location != \"\"";
	//const char* deletedQuery = "select count(*) from Headers where Location != \"\" and deleted = 1";
	const char* allMailQuery = "select count(*) from Headers";
	const char* deletedQuery = "select count(*) from Headers where deleted = 1";

	*email_total = 0;
	*deleted_emails = 0;

	if (sqlite3_open(filename, &db) != SQLITE_OK) {
		char msg[4096];
		snprintf(msg, 4096, "Unable to open sqlite3 DB %s", filename);
		MessageBox(global_hwnd, msg, "Error!", MB_OK);
		return;
	}

	if (sqlite3_prepare_v2(db, allMailQuery, -1, &stmt, NULL) != SQLITE_OK)
		goto outdb;

	if (sqlite3_step(stmt) != SQLITE_ROW)
		goto outstmt;

	*email_total = sqlite3_column_int(stmt, 0);

	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);

	if (sqlite3_prepare_v2(db, deletedQuery, -1, &stmt, NULL) != SQLITE_OK)
		goto outdb;

	if (sqlite3_step(stmt) != SQLITE_ROW)
		goto outstmt;

	*deleted_emails = sqlite3_column_int(stmt, 0);

outstmt:

	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);

outdb:

	sqlite3_close(db);

}

void Incredimail_2_Email_Count( const char *filename, int *email_total, int *deleted_emails ) {
char sql[MAX_CHAR];
char trimmed_filename[MAX_CHAR];
char containerID[MAX_CHAR];
char temp_dir[MAX_CHAR];
char container_path[MAX_CHAR];

const char *tail;
char *pdest;
int rc, del, deleted;
sqlite3 *db;
sqlite3_stmt *stmt;

   deleted = 0;

   memset( temp_dir, 0, sizeof(temp_dir) );
   memset( container_path, 0, sizeof(container_path) );
   strcpy( temp_dir, filename );
   pdest = strrchr( temp_dir, '\\' );
   strncpy( container_path, temp_dir, strlen( temp_dir ) - strlen( pdest ) );
   strcat( container_path, "\\Containers.db" );

   rc = sqlite3_open(container_path, &db);

   if( rc ) {
     // no printf....
     printf("can't open db\n");
   } else {
      // Zero out strings
      memset( sql, 0, sizeof(sql) );
      memset( trimmed_filename, 0, sizeof(trimmed_filename) );
      memset( containerID, 0, sizeof( containerID ) );

      // The filename minus the '.imm'
      strncpy( trimmed_filename, &pdest[1], strlen(pdest) );
      pdest = strrchr( trimmed_filename, '.' );
      trimmed_filename[strlen(trimmed_filename) - strlen(pdest)] = '\0';
   
      //sprintf(sql, "SELECT msgscount,containerID FROM CONTAINERS WHERE FILENAME='%s'", trimmed_filename);
	  const char* allMailQuery = "SELECT msgscount,containerID FROM CONTAINERS";
      *sql = allMailQuery;

      rc = sqlite3_prepare_v2( db, sql, (int) strlen( sql ), &stmt, &tail );

      // debug***************
      if( rc == SQLITE_OK ) {
         printf("OK!\n");
      }
      //*********************

      rc = sqlite3_step( stmt );

      // only get the first column and result
      *email_total = sqlite3_column_int(stmt,0);
      strcpy(containerID,sqlite3_column_text(stmt,1));

      // reset the sql statement
      sqlite3_reset( stmt );
      sqlite3_finalize( stmt );
      sqlite3_close( db );

      // setup for deleted emails
      rc = sqlite3_open(container_path, &db);
      memset( &sql, 0, sizeof(sql) );
      sprintf(sql, "SELECT Deleted FROM Headers WHERE containerID='%s'", containerID);
      rc = sqlite3_prepare_v2( db, sql, (int) strlen( sql ), &stmt, &tail );

      // debug***************
      if( rc == SQLITE_OK ) {
         printf("OK!\n");
      }
      //*********************
      rc = sqlite3_step( stmt );

      while( rc == SQLITE_ROW ) {
         del = sqlite3_column_int(stmt,0);
         printf("%s\n",sqlite3_column_text(stmt,0));
         if( del == 1 ) {
            deleted++;
         }
         rc = sqlite3_step( stmt );
      }
      sqlite3_reset( stmt );
      sqlite3_finalize( stmt );
   }

   // Incredimail 2 email total doesn't count delete emails
   *deleted_emails = deleted;
   *email_total += deleted;
   sqlite3_close( db );
}

void Incredimail_2_Get_Email_Offset_and_Size( const char *filename, unsigned int *file_offset, unsigned int *size, int email_index, int *deleted_email ) {

sqlite3 *db;
sqlite3_stmt *stmt;
char sql[MAX_CHAR];
char trimmed_filename[MAX_CHAR];
char containerID[MAX_CHAR];
char temp_dir[MAX_CHAR];
char container_path[MAX_CHAR];
const char *tail;
int i, rc;
char *pdest;

   memset( temp_dir, 0, sizeof(temp_dir) );
   memset( container_path, 0, sizeof(container_path) );
   strcpy( temp_dir, filename );
   pdest = strrchr( temp_dir, '\\' );
   strncpy( container_path, temp_dir, strlen( temp_dir ) - strlen( pdest ) );
   strcat( container_path, "\\Containers.db" );

   rc = sqlite3_open(container_path, &db);

   if( rc ) {
     // no printf....
     // printf("can't open db\n");
   } else {   
      // Zero out strings
      memset( sql, 0, sizeof(sql) );
      memset( trimmed_filename, 0, sizeof(trimmed_filename) );
      memset( containerID, 0, sizeof( containerID ) );

      // The filename minus the '.imm'
      strncpy( trimmed_filename, &pdest[1], strlen(pdest) );
      pdest = strrchr( trimmed_filename, '.' );
      trimmed_filename[strlen(trimmed_filename) - strlen(pdest)] = '\0';

      sprintf(sql, "SELECT containerID FROM CONTAINERS WHERE FILENAME='%s'", trimmed_filename);

      rc = sqlite3_prepare_v2( db, sql, (int) strlen( sql ), &stmt, &tail );

      // debug***************
      if( rc == SQLITE_OK ) {
          printf("OK!\n");
      }
      //*********************

      rc = sqlite3_step( stmt );

      // only get the first column and result
      strcpy(containerID,sqlite3_column_text(stmt,0));

      // reset the sql statement
      sqlite3_reset( stmt );
      sqlite3_finalize( stmt );
      sqlite3_close( db );   

      // setup next query
      rc = sqlite3_open(container_path, &db);
      memset( &sql, 0, sizeof(sql) );
      sprintf(sql, "SELECT MsgPos,LightMsgSize,Deleted FROM Headers WHERE containerID='%s' ORDER BY MsgPos ASC", containerID);
      rc = sqlite3_prepare_v2( db, sql, (int) strlen( sql ), &stmt, &tail );


      // debug***************
      //if( rc == SQLITE_OK ) {
      //   printf("OK!\n");
      //}
      //*********************
      rc = sqlite3_step( stmt );
      email_index--;  // the index has to be decremented for the correct index in the sqlite db

      // Loop though the index
      for(i = 0; i <= email_index; i++ ) {
         rc = sqlite3_step( stmt );
      }

      // I love sqlite conversions
      *file_offset   = sqlite3_column_int(stmt,0);
      *size          = sqlite3_column_int(stmt,1);
      *deleted_email = sqlite3_column_int(stmt,2);

      sqlite3_reset( stmt );
      sqlite3_finalize( stmt );
   }
   sqlite3_close( db );   
}