/* Nic Ballesteros, parser.c, CS 24000, Fall 2020
 * Last updated November 8, 2020
 */

#include "parser.h"
#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>

uint8_t g_running_status = 0x80;

/*
 *  parse_file recieves a midi filename as its argument and opens that midi
 *  file. parse_file calls smaller parsing functions to fully parse the file's
 *  chunks. The function returns a fully populated song_data_t with all the
 *  data of the song from the file.
 */

song_data_t * parse_file(const char * filename) {
  //assert that the char that was passed to the function isn't NULL.
  assert(filename);

  //open the file and store it in the pointer midi_file.
  FILE * midi_file = fopen(filename, "rb");

  //assert that the file was opened without errors.
  assert(midi_file);

  fseek(midi_file, 0, SEEK_END);

  //find the size of the file.
  int file_size = ftell(midi_file);

  //go to the beginning of the file.
  fseek(midi_file, 0, SEEK_SET);

  //allocate memory to store a new song_data_t.
  song_data_t * song_data = malloc(sizeof(song_data_t));

  //protective programming.
  song_data->track_list = NULL;

  //find the length of the filename.
  size_t filename_len = strlen(filename) + 1;

  //assert that song_data is not NULL.
  assert(song_data);

  //allocate memory on heap to store the filepath.
  char * filepath = malloc(sizeof(char) * filename_len);

  //assert that there were no errors.
  assert(filepath);

  //copy the data from filename to filepath.
  strncpy(filepath, filename, filename_len);

  song_data->path = filepath;

  //parse the header of the file.
  parse_header(midi_file, song_data);

  //while the file has not reached the end of the file, parse the next track.
  while ((ftell(midi_file) < file_size)) {
    parse_track(midi_file, song_data);
  }

  //assert that there is no more data in the file.
  assert(ftell(midi_file) == file_size);

  //close the midi_file.
  fclose(midi_file);

  //set the FILE pointer to NULL as a protective programming practice.
  midi_file = NULL;

  return song_data;
} /* parse_file() */

/*
 *  parse_header parses the header of a midi_file. The function populates the
 *  correct fields within song_data and when finished returns.
 */

void parse_header(FILE * midi_file, song_data_t * song_data) {
  //assert that the midi_file has been opened.
  assert(midi_file);

  //assert that song_data actually points to something.
  assert(song_data);

  //make a string to store the first 4 chars in the file.
  char chunk_type[5];

  //read in the first 4 chars.
  assert(fread(chunk_type, sizeof(char), 4, midi_file) == 4);

  //NUL terminate the string.
  chunk_type[4] = '\0';

  //assert that the first 4 chars in the file match "MThd".
  assert(strcmp(chunk_type, "MThd") == 0);

  //get the length of the midi
  uint8_t midi_len_bytes[4];

  //populate midi_len_bytes from the file. 4 uint8_t is the same as one uint32_t
  assert(fread(&midi_len_bytes, sizeof(uint8_t), 4, midi_file) == 4);

  //get a little-endian number from the big-endian length.
  uint32_t midi_len = end_swap_32(midi_len_bytes);

  //the header length should always be 6 bytes.
  assert(midi_len == 6);

  //save the individual bytes in arrays in big endian format.
  uint8_t format[2];
  uint8_t num_tracks[2];
  uint8_t division_bytes[2];

  //read in the next 6 bytes of data and set them to the correct pointers.
  assert(fread(format, sizeof(uint8_t), 2, midi_file) == 2);
  assert(fread(num_tracks, sizeof(uint8_t), 2, midi_file) == 2);
  assert(fread(division_bytes, sizeof(uint8_t), 2, midi_file) == 2);

  //convert the numbers into little-endian format.
  song_data->num_tracks = end_swap_16(num_tracks);
  song_data->division.ticks_per_qtr = end_swap_16(division_bytes);

  //get the current location of the file.
  int current_location = ftell(midi_file);

  //store the next track header which is 4 chars.
  char next_header[5];

  assert(fread(next_header, sizeof(char), 4, midi_file) == 4);

  next_header[4] = '\0';

  //assert that the next chunk is a track and the parser is still lined up.
  assert(strcmp(next_header, "MTrk") == 0);

  //go back to the top of the track.
  fseek(midi_file, current_location, SEEK_SET);

  //since the format can be one of 3 values only one byte of the two is
  //required and that is why I do not swap the values.
  assert(format[1] < 3);

  //assert that 6 bytes were read in.
  assert((sizeof(format) + sizeof(song_data->num_tracks) +
                           sizeof(song_data->division.ticks_per_qtr)) == 6);

  //format[0] is just going to be 0x00. Insignificant.
  song_data->format = format[1];

  //save tpq in a temp_int to find if the most significant bit is 1.
  uint16_t temp_int = song_data->division.ticks_per_qtr;

  //find if the 15th bit is 1 or 0.
  temp_int = temp_int >> 15;
  temp_int = temp_int & 0x0001;

  //if the largest bit is 1.
  if (temp_int == 1) {
    //the song does not use tpq.
    song_data->division.uses_tpq = false;

    //save the tpq as a local var.
    uint16_t tpq = song_data->division.ticks_per_qtr;

    //get the least significant 8 bits. This is the ticks_per_frame.
    uint8_t ticks_per_frame = tpq & 0xFF; // & 11111111

    //get rid of the bottom 8 bits.
    uint8_t frames_per_sec = tpq >> 8;

    //get rid of the 15th bit.
    frames_per_sec = tpq & 0x7F;

    //set the calculated values into the union for division in song_data.
    song_data->division.ticks_per_frame = ticks_per_frame;
    song_data->division.frames_per_sec = frames_per_sec;
  } else {
    //the song_data uses tpq.
    song_data->division.uses_tpq = true;
  }
} /* parse_header() */

/*
 *  parse_track parses a single track and populates song_data with that newly
 *  parsed track. parse_track uses parse_event to parse each event within the
 *  track. The function continues until the 0xFF 0x2F 0x00 event is hit, which
 *  is the "End of Track" meta event.
 */

void parse_track(FILE * midi_file, song_data_t * song_data) {
  //assert that the midi file has been opened.
  assert(midi_file);

  //assert that song_data is not NULL.
  assert(song_data);

  //store the chunk header.
  char chunk_header[5];

  //read the first 4 bytes of the chunk.
  assert(fread(chunk_header, sizeof(char), 4, midi_file) == 4);

  //NUL terminate the string.
  chunk_header[4] = '\0';

  //assert that the chunk header is of the right kind.
  assert(strcmp(chunk_header, "MTrk") == 0);

  //allocate memory for a new track list element.
  track_node_t * track_node = malloc(sizeof(track_node_t));

  //allocate memory for a new track data point.
  track_t * track = malloc(sizeof(track_t));

  //assert that memory was allocated without errors.
  assert(track);
  assert(track_node);

  //store the byte length of the track as an array in big endian format.
  uint8_t length[4];

  //get the length of the track and store it in track. length is 32 bits or
  //4 uint8_t values.
  assert(fread(length, sizeof(uint8_t), 4, midi_file) == 4);

  //order bytes in memory differently from file in little endian.
  track->length = end_swap_32(length);

  //if the length of the track is greater than 0.
  if (length > 0) {
    //set up the head of the event list.
    event_node_t * head = malloc(sizeof(event_node_t));

    //assert that malloc succeeded.
    assert(head);

    //set the head event to a new parsed event from the midi_file.
    head->event = parse_event(midi_file);

    //set the prev_event as the head of the list.
    event_node_t * prev_event = head;

    //fill the rest of the list
    while (1) {
      //allocate memory for a new node
      event_node_t * event_node = malloc(sizeof(event_node_t));

      assert(event_node);

      //get all the events for the track.
      event_node->event = parse_event(midi_file);

      //set the next event to NULL.
      event_node->next_event = NULL;

      //link the new node to the list and make the node the previous event.
      prev_event->next_event = event_node;
      prev_event = prev_event->next_event;

      //see if the parsed event is the META event 0x2F. This is the end of
      //the track and this loop should break.
      if (event_node->event->type == META_EVENT) {
        if (strcmp(event_node->event->midi_event.name, "End of Track") == 0) {
          //the End of Track meta event has been called
          break;
        }
      }
    }

    //set the track event_list to the newly created list.
    track->event_list = head;
  }

  //set the track of track_node as track.
  track_node->track = track;

  track_node->next_track = NULL;

  //if the list is not initialized, initialize it.
  if (song_data->track_list == NULL) {
    //set the first element of song_data->track_list as track_node.
    song_data->track_list = track_node;
  } else {
    //set the current node to the first element of song_data->track_list.
    track_node_t * current_node = song_data->track_list;

    //go to the end of the list.
    while (current_node->next_track) {
      current_node = current_node->next_track;
    }

    //set the next element as track_node.
    current_node->next_track = track_node;
  }
} /* parse_track() */

/*
 *  parse_event parses a single event from the midi_file. parse_event returns
 *  a populated event_t on the heap.
 */

event_t * parse_event(FILE * midi_file) {
  //assert that the midi_file has been opened.
  assert(midi_file);

  //allocate memory for a new event.
  event_t * event = malloc(sizeof(event_t));

  //assert that there were no errors in allocation.
  assert(event);

  //get the delta time value.
  event->delta_time = parse_var_len(midi_file);

  //store the first byte of the event.
  uint8_t event_first_byte = 0;

  //get the next byte that is the event type.
  assert(fread(&event_first_byte, sizeof(uint8_t), 1, midi_file) == 1);

  //determine the event type.
  if (event_first_byte == SYS_EVENT_1) {
    //populate the sys_event part of the event_t.
    event->sys_event = parse_sys_event(midi_file);
    event->type = SYS_EVENT_1;
  } else if (event_first_byte == SYS_EVENT_2) {
    //populate the sys_event part of the event_t.
    event->sys_event = parse_sys_event(midi_file);
    event->type = SYS_EVENT_2;
  } else if (event_first_byte == META_EVENT) {
    //populate the meta_event part of the event_t.
    event->meta_event = parse_meta_event(midi_file);
    event->type = META_EVENT;
  } else {
    //populate the midi_event part of the event_t.
    event->midi_event = parse_midi_event(midi_file, event_first_byte);
    event->type = event->midi_event.status;
  }

  return event;
} /* parse_event() */

/*
 *  parse_sys_event parses a sys_event from the midi_file. The function reads
 *  a VLQ and reads that many bytes into the sys_event.
 */

sys_event_t parse_sys_event(FILE *midi_file) {
  //assert that midi_file has been opened.
  assert(midi_file);

  //make a variable that we can assign values to and then return it.
  sys_event_t sys_event = (sys_event_t) {0};

  //assign the data len field.
  sys_event.data_len = parse_var_len(midi_file);

  if (sys_event.data_len > 0) {
    //assign the data field.
    sys_event.data = malloc(sizeof(uint8_t) * sys_event.data_len);

    //assert that there were no errors in allocation.
    assert(sys_event.data);

    //read the data from the midi file into memory.
    assert(fread(sys_event.data, sizeof(uint8_t), sys_event.data_len,
      midi_file) == sys_event.data_len);
  } else {
    sys_event.data = NULL;
  }

  return sys_event;
} /* parse_sys_event() */

/*
 *  parse_meta_event parses a meta event from the file and returns a newly
 *  populated meta_event. The function finds the type of meta event it is
 *  and depending if it has data or not, it will read in that data and allocate
 *  enough memory for it.
 */

meta_event_t parse_meta_event(FILE * midi_file) {
  //assert that midi_file was opened before the function
  assert(midi_file);

  uint8_t name_byte = 0;

  //read in one byte from file to find the type of meta event.
  assert(fread(&name_byte, sizeof(uint8_t), 1, midi_file) == 1);

  //get the basic info for the meta event.
  meta_event_t meta_event = META_TABLE[name_byte];

  //assert that the META_TABLE has an entry for the.
  assert(meta_event.name != NULL);

  //get the var length of the event
  uint32_t data_len = parse_var_len(midi_file);

  if (meta_event.data_len != 0) {
    assert(data_len == meta_event.data_len);
  } else {
    meta_event.data_len = data_len;
  }

  if (meta_event.data_len > 0) {
    //allocate memory to store the data of the event.
    meta_event.data = malloc(sizeof(uint8_t) * meta_event.data_len);

    //assert that malloc succeeded.
    assert(meta_event.data);

    //read in the data from the file
    assert(fread(meta_event.data, sizeof(uint8_t), meta_event.data_len,
      midi_file) == meta_event.data_len);
  } else {
    meta_event.data = NULL;
  }

  return meta_event;
} /* parse_meta_event() */

/*
 *  parse_midi_event parses a midi event from the midi_file. The first byte
 *  of the event is passed to the function to check that it is not a status
 *  byte. The first byte of the event is a status byte if the 7th bit is a 1.
 *  The function then determines the size of the data and allocates the
 *  correct amount of memory. The function then reads in the data and puts it
 *  into that allocated memory.
 */

midi_event_t parse_midi_event(FILE * midi_file, uint8_t first_byte) {
  //assert that the midi_file has been opened before function call
  assert(midi_file);

  //assert that the running status has the 7th bit as a 1.
  assert(g_running_status >= 0x80);

  midi_event_t midi_event = (midi_event_t) {0};

  //if the first byte is larger or equal to 0x80 the largest bit is 1 and
  //therefore it is a status message.
  if (first_byte >= 0x80) {
    g_running_status = first_byte;
  } else {
    //go back one byte because the event does not start with a status byte.
    int current_position = ftell(midi_file);

    fseek(midi_file, current_position - 1, SEEK_SET);
  }

  //assert that the g_running_status is a valid midi running status.
  assert(g_running_status <= 0xFE);
  assert(g_running_status != 0xF7);
  assert(g_running_status != 0xF0);

  //get the information about a certain midi event from the predefined
  //midi event table.
  midi_event = MIDI_TABLE[g_running_status];

  //if the midi event should have more data
  if (midi_event.data_len > 0) {
    //allocate the correct amount of data for the event.
    midi_event.data = malloc(sizeof(uint8_t) * midi_event.data_len);

    //assert that malloc succeeded.
    assert(midi_event.data);

    //read in all the bytes that are a part of the data of the event.
    assert(fread(midi_event.data, sizeof(uint8_t), midi_event.data_len,
      midi_file) == midi_event.data_len);
  } else {
    midi_event.data = NULL;
  }

  return midi_event;
} /* parse_midi_event() */

/*
 *  parse_var_len reads in a VLQ value from a midi file and returns that value
 *  as a uint32_t. If the byte is larger than 0x7F, then there is another byte
 *  that needs to be read from the file to complete the total value.
 */

uint32_t parse_var_len(FILE * midi_file) {
  //assert that the midi_file has been opened.
  assert(midi_file);

  uint32_t len = 0;
  uint8_t byte = 0;

  do {
    //read in one byte at a time.
    assert(fread(&byte, sizeof(uint8_t), 1, midi_file) == 1);

    //move the previously put 7 bits, 7 bits higher in the number.
    len = len << 7;

    //remove the continuation bit.
    len += byte & 0x7F;

  //continue while the continuation bit is 1.
  } while (byte >= 0x80);

  return len;
} /* parse_var_len() */

/*
 *  event_type determines the type of event an event is. If that event has a
 *  type 0xFF then it is a meta event. If it is type 0xF0 or 0xF7, then it is
 *  sysex event and otherwise it is a midi event.
 */

uint8_t event_type(event_t * event) {
  //assert that the argument exists.
  assert(event);

  //determine the event type.
  if ((event->type == SYS_EVENT_1) || (event->type == SYS_EVENT_2)) {
    //sysex event
    return SYS_EVENT_T;
  } else if (event->type == META_EVENT) {
    //meta event
    return META_EVENT_T;
  } else {
    //midi event
    return MIDI_EVENT_T;
  }
} /* event_type() */

/*
 *  free_song deallocates all the memory associated with a song_data_t.
 *  free_song uses other free functions to free the tracks and the events
 *  within those tracks.
 */

void free_song(song_data_t * song_data) {
  //assert that a non-NULL pointer was passed to the function.
  assert(song_data);

  //free the path string.
  free(song_data->path);

  //if the song is empty
  if (!song_data->track_list) {
    free(song_data);
    return;
  }

  //free all the tracks in the song.
  track_node_t * prev_node = NULL;
  track_node_t * current_node = song_data->track_list;

  //iterate through the track list and free the node before the current node.
  while (current_node->next_track) {
    //set the previous node as the current node.
    prev_node = current_node;

    //set the current node to the next node in the list.
    current_node = current_node->next_track;

    //free the previous node.
    free_track_node(prev_node);
  }

  //free the very last element in the track list.
  free_track_node(current_node);

  //free the song data.
  free(song_data);
} /* free_song() */

/*
 *  free_track_node deallocates all memory associated with a track. This
 *  function uses other free functions to fully deallocate each event within
 *  the track.
 */

void free_track_node(track_node_t * track_node) {
  //assert that track_node is not NULL
  assert(track_node);

  //free all events in the track
  event_node_t * prev_event = NULL;
  event_node_t * current_node = track_node->track->event_list;

  //iterate through the event list.
  while (current_node->next_event) {
    //store the location of the current node.
    prev_event = current_node;

    //iterate to the next node in the list.
    current_node = current_node->next_event;

    //free the previously stored location for the current node.
    free_event_node(prev_event);
  }

  //free the very last node in the list.
  free_event_node(current_node);

  //protective programming.
  track_node->track->event_list = NULL;

  //free the track.
  free(track_node->track);

  //free the whole node.
  free(track_node);
} /* free_track_node() */

/*
 *  free_event_node deallocates all memory associated with an event. The
 *  function determines the type of event the event passed as an argument is
 *  and frees the allocated data for that specific type of event.
 */

void free_event_node(event_node_t * event_node) {
  //assert that the pointer is not NULL.
  assert(event_node);

  //determine the type of event the argument is.
  if ((event_node->event->type == SYS_EVENT_1) ||
    (event_node->event->type == SYS_EVENT_2)) {
    //free the sys event data
    if (event_node->event->sys_event.data != NULL) {
      free(event_node->event->sys_event.data);
      event_node->event->sys_event.data = NULL;
    }
  } else if (event_node->event->type == META_EVENT) {
    //free the meta event data
    if (event_node->event->meta_event.data != NULL) {
      free(event_node->event->meta_event.data);
      event_node->event->meta_event.data = NULL;
    }
  } else {
    //free the midi event data
    if (event_node->event->midi_event.data != NULL) {
      free(event_node->event->midi_event.data);
      event_node->event->midi_event.data = NULL;
    }
  }

  //free the event.
  free(event_node->event);

  //free the event_node_t.
  free(event_node);
} /* free_event_node() */

/*
 *  end_swap_16 takes in an array of 2 bytes and bit shifts these bytes into a
 *  uint16_t which uses 2 bytes. The computer will automatically put these
 *  bytes into little endian because it's bitshifts already do this at runtime.
 */

uint16_t end_swap_16(uint8_t bytes[2]) {
  //set the final answer to the second byte of the int.
  uint16_t ans = bytes[0];

  //bitshift the most significant bit into the second byte.
  ans = ans << 8;

  //add the lower byte into the lower byte position.
  ans += bytes[1];

  return ans;
} /* end_swap_16() */

/*
 *  end_swap_32 does the same exact thing as end_swap_16, but instead changes
 *  4 bytes in big-endian form into little-endian form.
 */

uint32_t end_swap_32(uint8_t bytes[4]) {
  //where to store the final answer.
  uint32_t ans = bytes[0];

  //bitshift the most significant byte.
  ans = ans << 8;

  //set the second most significant byte.
  ans += bytes[1];

  //bitshift the two bytes up one more byte.
  ans = ans << 8;

  //set the third most significant byte.
  ans += bytes[2];

  //bitshift the 3 bytes to the next byte.
  ans = ans << 8;

  //set the fourth byte.
  ans += bytes[3];

  return ans;
} /* end_swap_32() */
