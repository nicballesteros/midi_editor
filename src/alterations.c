/* Nic Ballesteros, alterations.c, CS 24000, Fall 2020
 * Last updated October 12, 2020
 */

/* Add any includes here */

#include "alterations.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <malloc.h>

/*
 *  change_event_octave changes the octave that the notes are played in. The
 *  function finds all Midi Events that deal with notes and for each of those
 *  notes, it increments them by the number of octaves times the OCTAVE_STEP
 *  because there are 12 notes in an octave.
 */

int change_event_octave(event_t * event, int * num_octaves) {
  //get rid of all events except for midi events.
  if ((event->type != SYS_EVENT_1) && (event->type != SYS_EVENT_2) &&
      (event->type != META_EVENT)) {
    //save the status in a local variable
    uint8_t status = event->midi_event.status;

    //if the status byte is a Note On or Note Off or Polyphonic Key Pressure.
    if ((status >= 0x80) && (status <= 0xAF)) {
      int num_notes = *num_octaves * OCTAVE_STEP;

      //if the final value will be within the range for notes
      if ((num_notes + event->midi_event.data[0] < 0x80) &&
          (num_notes + event->midi_event.data[0] >= 0)) {
        //add the octave to the notes.
        event->midi_event.data[0] += num_notes;
        return 1;
      }
    }
  }
  //returns 0 if event was not modified.
  return 0;
} /* change_event_octave() */

/*
 *  change_event_time scales the delta time value of the event by the float
 *  value passed as an argument. The function returns the difference in bytes
 *  the new delta time takes up in VLQ format.
 */

int change_event_time(event_t * event, float * time_difference) {
  //find how many bytes delta time uses before the shift.
  uint32_t delta_time = event->delta_time;

  //store the original number of bytes.
  int original_num_of_bytes = 0;

  //if the delta time is 0, the VLQ uses 1 byte.
  if (delta_time == 0) {
    original_num_of_bytes++;
  }

  while (delta_time > 0) {
    //bitshift 7 right. If the resulting value is 0, there is no need for
    //another byte.
    delta_time = delta_time >> 7;

    //increment the required bytes needed to put delta_time in a VLQ.
    original_num_of_bytes++;
  }

  //if when scaled, the delta time exceeds 0x0FFFFFFF, the value is too large
  //to fit in a VLQ. Therefore, set the delta_time to the largest value.
  if (event->delta_time * *time_difference > 0x0FFFFFFF) {
    event->delta_time = 0x0FFFFFFF;
  } else {
    //scale delta_time
    event->delta_time = event->delta_time * *time_difference;
  }

  //save these values to calculate how many bytes in a VLQ for delta_time.
  delta_time = event->delta_time;
  int new_num_of_bytes = 0;

  //if the delta_time is 0, the VLQ uses only one byte.
  if (delta_time == 0) {
    new_num_of_bytes++;
  }

  while (delta_time > 0) {
    //bitshift 7 right. If the resulting value is 0, there is no need for
    //another byte.
    delta_time = delta_time >> 7;

    //increment the required bytes needed to put delta_time in a VLQ.
    new_num_of_bytes++;
  }

  //return the number of bytes delta time will be in VLQ format.
  return new_num_of_bytes - original_num_of_bytes;
} /* change_event_time() */

/*
 *  change_event_instrument changes the instrument for the event passed as an
 *  argument. Depending on what the old instrument was, the remapping will
 *  give the event some other instrument to use. The function returns 1 if the
 *  event was a Program Change event and 0 otherwise.
 */

int change_event_instrument(event_t * event, remapping_t new_instrument) {
  //get rid of all events except for midi events.
  if ((event->type != SYS_EVENT_1) && (event->type != SYS_EVENT_2) &&
      (event->type != META_EVENT)) {
    //get only program change midi events
    if ((event->midi_event.status >= 0xC0) &&
        (event->midi_event.status <= 0xCF)) {
      //map the old instrument to the new instrument.
      event->midi_event.data[0] = new_instrument[event->midi_event.data[0]];
      return 1;
    }
  }

  return 0;
} /* change_event_instrument() */

/*
 *  change_event_note changes the event's notes by using the remapping.
 *  Depending on what the old note was, a new note will be given to the event.
 *  If the event is a note event and the note was changed, the function returns
 *  1 and otherwise 0.
 */

int change_event_note(event_t * event, remapping_t new_notes) {
  //get rid of all events except for midi events.
  if ((event->type != SYS_EVENT_1) && (event->type != SYS_EVENT_2) &&
      (event->type != META_EVENT)) {
    //save the status in a local variable
    uint8_t status = event->midi_event.status;

    //if the status byte is a Note On or Note Off or Polyphonic Key Pressure.
    if ((status >= 0x80) && (status <= 0xAF)) {
      //map the old note to the new note.
      event->midi_event.data[0] = new_notes[event->midi_event.data[0]];
      return 1;
    }
  }

  return 0;
} /* change_event_note() */

/*
 *  apply_to_events applys a function passed in as an argument to each event
 *  in a song. Data is passed in as an argument to be used as an argument in
 *  the function that is called on each event of the song.
 */

int apply_to_events(song_data_t * song_data, event_func_t function,
  void * data) {
  //store the total return values.
  int sum_of_return_values = 0;

  //save the position of the track list.
  track_node_t * current_track = song_data->track_list;

  //iterate through the track list.
  while (current_track->next_track) {
    //save the position of the event list.
    event_node_t * current_event = current_track->track->event_list;

    //iterate through the event list.
    while (current_event->next_event) {
      //run the function on each event.
      sum_of_return_values += function(current_event->event, data);

      //iterate to the next event in the list.
      current_event = current_event->next_event;
    }

    //run the function on the last event.
    sum_of_return_values += function(current_event->event, data);

    //iterate to the next track in the list.
    current_track = current_track->next_track;
  }

  //run through the last track in the list.
  event_node_t * current_event = current_track->track->event_list;

  //iterate through each event in the list.
  while (current_event->next_event) {
    //run the function on each event in the list.
    sum_of_return_values += function(current_event->event, data);

    //iterate to the next event.
    current_event = current_event->next_event;
  }

  //run the function on the very last element in the event list.
  sum_of_return_values += function(current_event->event, data);

  //return the total number of the return values.
  return sum_of_return_values;
} /* apply_to_events() */

/*
 *  apply_to_track_events receives a track rather than a whole song like in
 *  apply_to_events. This function does the same thing apply_to_events does.
 */

int apply_to_track_events(track_t * track_data, event_func_t function,
  void * data) {
  //store the total return values.
  int sum_of_return_values = 0;

  //run through the last track in the list.
  event_node_t * current_event = track_data->event_list;

  //iterate through each event in the list.
  while (current_event->next_event) {
    //run the function on each event in the list.
    sum_of_return_values += function(current_event->event, data);

    //iterate to the next event.
    current_event = current_event->next_event;
  }

  //run the function on the very last element in the event list.
  sum_of_return_values += function(current_event->event, data);

  //return the total number of the return values.
  return sum_of_return_values;
} /* apply_to_track_events() */


/*
 *  change_octave takes in a song and changes the octave by the number of
 *  octaves passed in as an argument for each event in the song.
 */

int change_octave(song_data_t * song_data, int octaves) {
  //used to pass the function in as an argument.
  event_func_t change_each_event = (event_func_t)change_event_octave;

  //apply change_each_event to each event in the song.
  return apply_to_events(song_data, change_each_event, &octaves);
} /* change_octave() */

/*
 *  warp_time warps the time of the song passed as an argument. The delta_time
 *  of each event in the song is scaled by the float value passed as an
 *  argument.
 */

int warp_time(song_data_t * song_data, float warp) {
  //used to pass the function in as an argument.
  event_func_t change_each_event = (event_func_t)change_event_time;

  //get the number of elements in the track list.
  int number_of_tracks = 0;

  //save the position of the track list.
  track_node_t * current_track = song_data->track_list;

  //iterate through the track list.
  while (current_track) {
    current_track = current_track->next_track;
    number_of_tracks++;
  }

  current_track = song_data->track_list;

  //declare memory to store track nodes temporarily.
  track_node_t * track_list[number_of_tracks];

  //iterate through the list and insert the pointers into an array.
  for (int i = 0; i < number_of_tracks; i++) {
    track_list[i] = current_track;

    current_track = current_track->next_track;
  }

  int total_len_diff = 0;

  //apply the delta_time change to each event and get the byte difference.
  for (int i = 0; i < number_of_tracks; i++) {
    int track_len_diff = apply_to_track_events(track_list[i]->track,
      change_each_event, &warp);

    //add the difference of bytes to the length of the track.
    track_list[i]->track->length += track_len_diff;
    total_len_diff += track_len_diff;
  }

  return total_len_diff;
} /* warp_time() */

/*
 *  remap_instruments takes a whole song and remaps all the instruments of the
 *  song to a new set of instruments.
 */

int remap_instruments(song_data_t * song_data, remapping_t instrument_table) {
  //used to pass the function in as an argument.
  event_func_t change_each_event = (event_func_t)change_event_instrument;

  //apply change_each_event to each event in the song.
  //note_table is an array and therefore a pointer.
  return apply_to_events(song_data, change_each_event, instrument_table);
} /* remap_instruments() */

/*
 *  remap_notes takes a whole song and remaps all the notes of the song to a
 *  new set of notes.
 */

int remap_notes(song_data_t * song_data, remapping_t note_table) {
  //used to pass the function in as an argument.
  event_func_t change_each_event = (event_func_t)change_event_note;

  //apply change_each_event to each event in the song.
  //remapping is an array and therefore a pointer.
  return apply_to_events(song_data, change_each_event, note_table);
} /* remap_notes() */

/*
 *  copy_event takes an old event node. The function allocates memory for a
 *  new event node and a new event and copies all data from the old event to
 *  the new event. The function returns the pointer to this new event_node_t.
 */

event_node_t * copy_event(event_node_t * old_event_node) {
  //save the event within the node for better readability.
  event_t * old_event = old_event_node->event;

  //allocate memory for the new event node.
  event_node_t * new_event_node = malloc(sizeof(event_node_t));
  assert(new_event_node);

  event_t * new_event = malloc(sizeof(event_t));
  assert(new_event);

  //set up new_event_node.
  new_event_node->event = new_event;
  new_event_node->next_event = NULL;

  //copy the delta time to the new event.
  new_event->delta_time = old_event->delta_time;

  //copy the event type to the new event.
  new_event->type = old_event->type;

  //save where the event data is stored.
  uint8_t * old_data = NULL;
  uint8_t * new_data = NULL;

  //the length of the data.
  int data_length = 0;

  if ((old_event->type == SYS_EVENT_1) || (old_event->type == SYS_EVENT_2)) {
    //the event is a sys event.
    new_event->sys_event.data_len = old_event->sys_event.data_len;

    //allocate memory for the new event data.
    if (new_event->sys_event.data_len > 0) {
      new_event->sys_event.data = malloc(new_event->sys_event.data_len);
      assert(new_event->sys_event.data);
    } else {
      new_event->sys_event.data = NULL;
      return new_event_node;
    }

    //save the data pointers to copy them later.
    old_data = old_event->sys_event.data;
    new_data = new_event->sys_event.data;

    //save the length of the data.
    data_length = new_event->sys_event.data_len;
  } else if (old_event->type == META_EVENT) {
    //the first event is a meta event.
    new_event->meta_event.name = old_event->meta_event.name;

    //copy the data len into the new event.
    new_event->meta_event.data_len = old_event->meta_event.data_len;

    //allocate memory for the new event data.
    if (new_event->meta_event.data_len > 0) {
      new_event->meta_event.data = malloc(new_event->meta_event.data_len);
      assert(new_event->meta_event.data);
    } else {
      new_event->meta_event.data = NULL;
      return new_event_node;
    }

    //save pointers to the data.
    old_data = old_event->meta_event.data;
    new_data = new_event->meta_event.data;

    //save the length of the data.
    data_length = new_event->meta_event.data_len;
  } else {
    //the first event is a midi event.
    new_event->midi_event.name = old_event->midi_event.name;

    //copy the status byte to the new event.
    new_event->midi_event.status = old_event->midi_event.status;

    //copy the data len into the new event.
    new_event->midi_event.data_len = old_event->midi_event.data_len;

    //allocate memory for the new event data.
    if (new_event->midi_event.data_len > 0) {
      new_event->midi_event.data = malloc(new_event->midi_event.data_len);
      assert(new_event->midi_event.data);
    } else {
      new_event->midi_event.data = NULL;
      return new_event_node;
    }

    //save pointers to the data.
    old_data = old_event->midi_event.data;
    new_data = new_event->midi_event.data;

    //save the length of the data.
    data_length = new_event->midi_event.data_len;
  }

  //copy all data bytes to a new place in memory.
  for (int i = 0; i < data_length; i++) {
    new_data[i] = old_data[i];
  }

  return new_event_node;
} /* copy_event() */

/*
 *  add_round takes a song, a track index, an octave differential, a time delay,
 *  and a new instrument. The function uses these arguments to make a new track
 *  with these specifications. The function makes this new track have the
 *  difference in octave, the time delay compared to the original track, and
 *  makes all instruments the instrument passed as an argument.
 */

void add_round(song_data_t * song_data, int track_index,
  int octave_differential, unsigned int time_delay, uint8_t new_instrument) {
  //assert that the song data's format is not 2.
  assert(song_data->format != 2);

  /*
   *  Duplicate the track in song_data at track_index.
   */

  //save the position of the list.
  track_node_t * track_to_dupe = song_data->track_list;

  assert(track_to_dupe);

  //find the correct track.
  for (int i = 0; i < track_index; i++) {
    //all but the last element in the list have a next pointer
    track_to_dupe = track_to_dupe->next_track;

    assert(track_to_dupe);
  }

  //find what channel is open.
  char channels[16];

  //set the array values to 0.
  for (int i = 0; i < 16; i++) {
    channels[i] = 0;
  }

  track_node_t * current_track = song_data->track_list;

  //iterate through the track list to find all the channel messages.
  while (current_track->next_track) {
    event_node_t * current_event = current_track->track->event_list;

    //iterate through the event_list finding all the channel messages.
    while (current_event->next_event) {
      //find the events in the song that are channel messages.
      if ((current_event->event->type != SYS_EVENT_1) &&
          (current_event->event->type != SYS_EVENT_2) &&
          (current_event->event->type != META_EVENT)) {
        if ((current_event->event->midi_event.status >= 0x80) &&
            (current_event->event->midi_event.status <= 0xEF)) {
          //get the status byte of the channel message
          uint8_t channel = current_event->event->midi_event.status;

          //get the channel nibble.
          channel &= 0x0F;
          channels[channel] = 1;
        }
      }

      current_event = current_event->next_event;
    }

    //compare the last element in the list.
    if ((current_event->event->type != SYS_EVENT_1) &&
        (current_event->event->type != SYS_EVENT_2) &&
        (current_event->event->type != META_EVENT)) {
      if ((current_event->event->midi_event.status >= 0x80) &&
          (current_event->event->midi_event.status <= 0xEF)) {
        uint8_t channel = current_event->event->midi_event.status;

        //get the channel nibble.
        channel &= 0x0F;
        channels[channel] = 1;
      }
    }

    current_track = current_track->next_track;
  }

  //iterate through the event_list of the final track.
  event_node_t * current_event = current_track->track->event_list;

  //iterate through the event_list finding all the channel messages.
  while (current_event->next_event) {
    //get all channel message events.
    if ((current_event->event->type != SYS_EVENT_1) &&
        (current_event->event->type != SYS_EVENT_2) &&
        (current_event->event->type != META_EVENT)) {
      if ((current_event->event->midi_event.status >= 0x80) &&
          (current_event->event->midi_event.status <= 0xEF)) {
        uint8_t channel = current_event->event->midi_event.status;

        //get the channel nibble.
        channel &= 0x0F;
        channels[channel] = 1;
      }
    }

    current_event = current_event->next_event;
  }

  //compare the last element in the list.
  if ((current_event->event->type != SYS_EVENT_1) &&
        (current_event->event->type != SYS_EVENT_2) &&
        (current_event->event->type != META_EVENT)) {
    if ((current_event->event->midi_event.status >= 0x80) &&
        (current_event->event->midi_event.status <= 0xEF)) {
      uint8_t channel = current_event->event->midi_event.status;

      //get the channel nibble.
      channel &= 0x0F;
      channels[channel] = 1;
    }
  }

  uint8_t index = 0;

  //find the index of the array that has a 0 in it.
  while ((channels[index] == 1) && (index < 15)) {
    index++;
  }

  uint8_t smallest_open_channel = index;

  //assert that the channel[index] == 0 because if not, there are no open
  //channels.
  assert(channels[index] == 0);

  //allocate memory for a new track.
  track_node_t * new_track_node = malloc(sizeof(track_node_t));
  assert(new_track_node);

  track_t * new_track = malloc(sizeof(track_t));
  assert(new_track);

  //copy the track meta data into the new track.
  new_track_node->track = new_track;
  new_track_node->next_track = NULL;

  new_track->length = track_to_dupe->track->length;

  //save the position of the event list.
  event_node_t * current_old_event = track_to_dupe->track->event_list;

  event_node_t * prev_new_event = NULL;
  event_node_t * current_new_event = NULL;

  while (current_old_event->next_event) {
    //copy the values in current_old_event to current_new_event.
    current_new_event = copy_event(current_old_event);

    //the first time the loop runs, set the head of the list to the
    //current_new_event.
    if (current_old_event == track_to_dupe->track->event_list) {
      new_track->event_list = current_new_event;
    }

    //if prev_new_event exists, link it to the current new event.
    if (prev_new_event) {
      prev_new_event->next_event = current_new_event;
    }

    //iterate to the next element to copy.
    prev_new_event = current_new_event;
    current_old_event = current_old_event->next_event;
  }

  //copy the last event to the new event list.
  current_new_event = copy_event(current_old_event);

  //if prev_new_event exists, link it to current_new_event.
  if (prev_new_event) {
    prev_new_event->next_event = current_new_event;
  }

  //add the new track to the track list.
  current_track = track_to_dupe;

  //iterate through the song_list until the very end of the list.
  while (current_track->next_track) {
    current_track = current_track->next_track;
  }

  //set the last element of the list to the new track node.
  current_track->next_track = new_track_node;

  //increment the number of tracks in the song.
  song_data->num_tracks += 1;

  /*
   *  Add round.
   */

  current_event = new_track->event_list;

  //signify that the last delta time does not exist.
  int last_delta_time = -1;

  //store the difference in bytes between the two tracks.
  int byte_length = 0;

  //iterate through the events and apply the time_delay.
  while (current_event->next_event) {
    //change the current event's octave.
    change_event_octave(current_event->event, &octave_differential);

    //set the current event if it is a program change event to the instrument.
    if ((current_event->event->type >= 0xC0) &&
        (current_event->event->type <= 0xCF)) {
      current_event->event->midi_event.data[0] = new_instrument;
    }

    int delta_time = current_event->event->delta_time;
    int original_num_of_bytes = 0;

    if (current_event->event->delta_time == 0) {
      original_num_of_bytes++;
    }

    while (delta_time > 0) {
      //bitshift 7 right. If the resulting value is 0, there is no need for
      //another byte.
      delta_time = delta_time >> 7;

      //increment the required bytes needed to put delta_time in a VLQ.
      original_num_of_bytes++;
    }

    //only apply the time delay to the event if it is the first element in the
    //event list, or the original delta time is greater than 0.
    if ((last_delta_time == -1) || (delta_time > 0)) {
      current_event->event->delta_time += time_delay;
    } else {
      //else continue to the next event in the list.
      current_event = current_event->next_event;
      continue;
    }

    //find the number of bytes the time delay adds to the VLQ value.
    int new_num_of_bytes = 0;

    delta_time = current_event->event->delta_time;

    //set the last delta time to the new delta time.
    last_delta_time = delta_time;

    if (current_event->event->delta_time == 0) {
      new_num_of_bytes++;
    }

    while (delta_time > 0) {
      //bitshift 7 right. If the resulting value is 0, there is no need for
      //another byte.
      delta_time = delta_time >> 7;

      //increment the required bytes needed to put delta_time in a VLQ.
      new_num_of_bytes++;
    }

    //add the difference of the new delta time bytes to the total count.
    byte_length += new_num_of_bytes - original_num_of_bytes;

    //go to the next event.
    current_event = current_event->next_event;
  }

  //change the event octave for each event.
  change_event_octave(current_event->event, &octave_differential);

  //set the current event if it is a program change to the new instrument.
  if ((current_event->event->type >= 0xC0) &&
        (current_event->event->type <= 0xCF)) {
    current_event->event->midi_event.data[0] = new_instrument;
  }

  int delta_time = current_event->event->delta_time;

  int original_num_of_bytes = 0;

  if (current_event->event->delta_time == 0) {
    original_num_of_bytes++;
  }

  while (delta_time > 0) {
    //bitshift 7 right. If the resulting value is 0, there is no need for
    //another byte.
    delta_time = delta_time >> 7;

    //increment the required bytes needed to put delta_time in a VLQ.
    original_num_of_bytes++;
  }

  //current_event->event->delta_time += time_delay;
  if ((last_delta_time == -1) || (delta_time > 0)) {
    current_event->event->delta_time += time_delay;
  }

  int new_num_of_bytes = 0;

  delta_time = current_event->event->delta_time;

  if (current_event->event->delta_time == 0) {
    new_num_of_bytes++;
  }

  while (delta_time > 0) {
    //bitshift 7 right. If the resulting value is 0, there is no need for
    //another byte.
    delta_time = delta_time >> 7;

    //increment the required bytes needed to put delta_time in a VLQ.
    new_num_of_bytes++;
  }

  byte_length += new_num_of_bytes - original_num_of_bytes;

  //update the length of the track.
  new_track->length += byte_length;

  current_event = new_track->event_list;

  //if the event is a channel message, change the channel to the smallest open
  //channel.
  while (current_event->next_event) {
    if ((current_event->event->type != SYS_EVENT_1) &&
        (current_event->event->type != SYS_EVENT_2) &&
        (current_event->event->type != META_EVENT)) {
      if ((current_event->event->type >= 0x80) &&
          (current_event->event->type <= 0xEF)) {
        current_event->event->type &= 0xF0;
        current_event->event->type += smallest_open_channel;

        current_event->event->midi_event.status = current_event->event->type;
      }

    }
    current_event = current_event->next_event;
  }

  if ((current_event->event->type != SYS_EVENT_1) &&
      (current_event->event->type != SYS_EVENT_2) &&
      (current_event->event->type != META_EVENT)) {
    if ((current_event->event->type >= 0x80) &&
        (current_event->event->type <= 0xEF)) {
      current_event->event->type &= 0xF0;
      current_event->event->type += smallest_open_channel;

      current_event->event->midi_event.status = current_event->event->type;
    }

  }

  //regardless of what the format was, it should now be of format 1.
  song_data->format = 1;
} /* add_round() */

/*
 * Function called prior to main that sets up random mapping tables
 */

void build_mapping_tables()
{
  for (int i = 0; i <= 0xFF; i++) {
    I_BRASS_BAND[i] = 61;
  }

  for (int i = 0; i <= 0xFF; i++) {
    I_HELICOPTER[i] = 125;
  }

  for (int i = 0; i <= 0xFF; i++) {
    N_LOWER[i] = i;
  }
  //  Swap C# for C
  for (int i = 1; i <= 0xFF; i += 12) {
    N_LOWER[i] = i - 1;
  }
  //  Swap F# for G
  for (int i = 6; i <= 0xFF; i += 12) {
    N_LOWER[i] = i + 1;
  }
} /* build_mapping_tables() */
