/* Nic Ballesteros, ui.c, CS 24000, Fall 2020
 * Last updated October 12, 2020
 */

/* Add any includes here */
#include "ui.h"
#include "song_writer.h"
#include <malloc.h>
#include <assert.h>
#include <stdio.h>
#include <alterations.h>

tree_node_t * g_current_node = NULL;
song_data_t * g_current_song = NULL;
song_data_t * g_modified_song = NULL;

/* Widgets */
//declarations of all Gtk Widgets.
GtkWidget * window;
GtkWidget * grid;

//left side of the window
//GtkWidget * left_grid;

GtkWidget * add_file_button;
GtkWidget * add_file_button_box;

GtkWidget * load_dir_button;
GtkWidget * load_dir_button_box;

GtkWidget * list_box;

int g_list_box_elements;

GtkWidget * search_bar;
GtkWidget * search_entry;

GtkWidget * window_separator;

//right side of the window
GtkWidget * right_grid;

GtkWidget * info_label1;
GtkWidget * info_label2;
GtkWidget * info_label3;
GtkWidget * info_label4;

GtkWidget * t_scale_label;
GtkWidget * t_scale;

gint note_width;

GtkWidget * original_grid;
GtkWidget * after_grid;

GtkWidget * original_song_frame;
GtkWidget * original_draw_area;

GtkWidget * after_draw_area;
GtkWidget * after_effect_frame;

GtkWidget * original_scroll;
GtkWidget * after_scroll;

GtkWidget * save_song_box;
GtkWidget * save_song_button;

GtkWidget * remove_song_box;
GtkWidget * remove_song_button;

GtkWidget * warp_time_label;
GtkWidget * warp_time_button;

GtkWidget * change_octave_label;
GtkWidget * change_octave_button;

GtkWidget * remap_instruments_label;
GtkWidget * remap_instruments_list;

GtkWidget * remap_notes_label;
GtkWidget * remap_notes_list;

gdouble time_warp;
gint octave_change;

/*
 *  populate_list_from_tree is a helper function that is called on every
 *  element in g_song_library. It can be used in traverse_in_order,
 *  traverse_pre_order, and traverse_post_order.
 */

void populate_list_from_tree(tree_node_t * root, void * list_box) {
  //cast the list_box from a void to a GtkWidget.
  GtkWidget * list_box_widget = GTK_WIDGET(list_box);

  //make a new row and label for this root.
  GtkWidget * list_box_row = gtk_list_box_row_new();
  GtkWidget * list_label = gtk_label_new(root->song_name);

  //make the label align to the left.
  gtk_label_set_xalign(GTK_LABEL(list_label), 0.0);

  //g_signal_connect(list_box_row, "row-activated", G_CALLBACK(song_selected_cb), 
    //NULL);


  //add the label to the list_box_row.
  gtk_container_add(GTK_CONTAINER(list_box_row), list_label);

  //insert the row into the list.
  gtk_list_box_insert(GTK_LIST_BOX(list_box_widget), list_box_row, g_list_box_elements);

  //increment the number of elements in the list.
  g_list_box_elements++;
}

/*
 *  update_song_list updates the song list on the left of the window. It
 *  destroys the old list and creates a new one. It populates it from
 *  g_song_library.
 */

void update_song_list() {
  //g_print("update_song_list\n");
  //destroy the old list_box
  gtk_widget_destroy(list_box);

  //make a new list_box.
  list_box = gtk_list_box_new();

  //attach an event listener to call the song_selected_cb.
  g_signal_connect(list_box, "row-activated", G_CALLBACK(song_selected_cb), 
    NULL);

  //attach the list_box in the correct grid area.
  gtk_grid_attach(GTK_GRID(grid), list_box, 0, 1, 2, 1);

  //size the new list_box at 700 pixels height.
  gtk_widget_set_size_request(list_box, -1, 740);

  //set the number of elements in the list to 0.
  g_list_box_elements = 0;

  //make a function pointer to run on each element of the tree.
  traversal_func_t func = &populate_list_from_tree;

  //get all the elements of the tree.
  traverse_in_order(g_song_library, (void*)list_box, func);

  //show the newly made list.
  gtk_widget_show_all(grid);
}

/*
 *  update_drawing_area
 */

void update_drawing_area() {
  //g_print("update_drawing_area\n");
  gint original_width = gtk_widget_get_allocated_width(original_draw_area);
  gint original_height = gtk_widget_get_allocated_height(original_draw_area);

  gint after_width = gtk_widget_get_allocated_width(after_draw_area);
  gint after_height = gtk_widget_get_allocated_height(after_draw_area);

  g_signal_connect(G_OBJECT(original_draw_area), "draw", G_CALLBACK(draw_cb), g_current_song);
  g_signal_connect(G_OBJECT(after_draw_area), "draw", G_CALLBACK(draw_cb), g_modified_song);

  gtk_widget_queue_draw_area(original_draw_area, 0, 0, original_width,
    original_height);

  gtk_widget_queue_draw_area(after_draw_area, 0, 0, after_width,
    after_height);
}

/*
 *  update_info
 */

void update_info() {
  //GtkListBoxRow * row = gtk_list_box_get_selected_row(GTK_LIST_BOX(list_box));
  //g_print("update_info()\n");
  if (g_current_song == NULL) {
    //g_print("NULLLLLLL\n");
    gtk_label_set_text(GTK_LABEL(info_label1),
      "Select a file from list to start ...");
    gtk_label_set_text(GTK_LABEL(info_label2), "");
    gtk_label_set_text(GTK_LABEL(info_label3), "");
    gtk_label_set_text(GTK_LABEL(info_label4), "");
    return;
  }

  int lowest_note = 0;
  int highest_note = 0;
  int song_length = 0;

  range_of_song(g_current_song, &lowest_note, &highest_note, &song_length);

  //the string can only ever be 9 chars in length.
  char note_range[22] = { 0 };

  sprintf(note_range, "Note range: [%d,%d]", lowest_note, highest_note);

  //g_print("%s\n", note_range);

  note_range[21] = '\0';

  char song_length_str[32];

  sprintf(song_length_str, "Original length: %d", song_length);

  song_length_str[31] = '\0';

  gtk_label_set_text(GTK_LABEL(info_label1), g_current_node->song_name);
  gtk_label_set_text(GTK_LABEL(info_label2), g_current_song->path);
  gtk_label_set_text(GTK_LABEL(info_label3), note_range);
  gtk_label_set_text(GTK_LABEL(info_label4), song_length_str);

  //g_print("all good here\n");

  update_drawing_area();
}

/*
 *  update_song
 */

void update_song() {
  //g_print("update_song()\n");
  if (g_current_song) {
    if (g_modified_song) {
      //g_print("deleted_song\n");
      free_song(g_modified_song);
    }

    //g_print("update_song");
    g_modified_song = parse_file(g_current_song->path);

    assert(g_modified_song != NULL);

    if (time_warp != 1.0) {
      warp_time(g_modified_song, time_warp);
    }

    if (octave_change != 0) {
      change_octave(g_modified_song, octave_change);
    }

    //redraw the second draw area with the modified song.
    gint after_width = gtk_widget_get_allocated_width(after_draw_area);
    gint after_height = gtk_widget_get_allocated_height(after_draw_area);

    g_signal_connect(G_OBJECT(after_draw_area), "draw", G_CALLBACK(draw_cb), g_modified_song);

    gtk_widget_queue_draw_area(after_draw_area, 0, 0, after_width,
      after_height);
    
  }

  update_drawing_area();
} /* update_song() */

/*
 *  range_of_song
 */

void range_of_song(song_data_t * song_data, int * lowest, int * highest,
  int * song_length) {
  //assert that the song data is not NULL.
  if (!song_data) {
    return;
  }


  assert(song_data);

  //set the lowest and highest value to the highest and lowest possible notes.
  if (lowest) {
    *lowest = 0x80;
  }

  if (highest) {
    *highest = 0x00;
  }

  if (song_length) {
    *song_length = 0x00;
  }

  track_node_t * current_track = song_data->track_list;

  event_node_t * current_event = NULL;

  while (current_track) {
    current_event = current_track->track->event_list;

    int this_track_length = 0;

    while (current_event) {
      //only get the events that deal with note events.
      if ((current_event->event->type >= 0x80) &&
          (current_event->event->type <= 0xAF)) {
        if (lowest) {
          if (current_event->event->midi_event.data[0] < *lowest) {
            *lowest = current_event->event->midi_event.data[0];
          }
        }

        if (highest) {
          if (current_event->event->midi_event.data[0] > *highest) {
            *highest = current_event->event->midi_event.data[0];
          }
        }
      }

      if (current_event->event->delta_time > 0) {
        this_track_length += current_event->event->delta_time;
      }

      current_event = current_event->next_event;
    }
    if (song_length) {
      if (this_track_length > *song_length) {
        *song_length = this_track_length;
      }
    }

    current_track = current_track->next_track;
  }
}

//void print_hello();
/*
void print_hello (GtkWidget *widget,
             gpointer   data) {
  g_print ("Hello World\n");
}
*/
/*
 *  activate
 */

void activate(GtkApplication * app, gpointer ptr) {
  //set up the window and set its properties.
  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW (window), "MIDI Library");

  gtk_window_set_default_size (GTK_WINDOW (window), 950, 800);
  gtk_container_set_border_width(GTK_CONTAINER (window), 10);
  gtk_window_set_resizable(GTK_WINDOW (window), FALSE);

  //set up the grid.
  grid = gtk_grid_new();
  gtk_container_add(GTK_CONTAINER (window), grid);

  gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

  //set up add_file_button.
  add_file_button_box = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);

  //add_file_button_box:child-min-width = 150;

  add_file_button = gtk_button_new_with_label ("Add File from File");
  g_signal_connect (add_file_button, "clicked", G_CALLBACK (add_song_cb), window);
  gtk_container_add (GTK_CONTAINER (add_file_button_box), add_file_button);

  gtk_grid_attach(GTK_GRID(grid), add_file_button_box, 0, 0, 1, 1);

  //set up load_dir_button
  load_dir_button_box = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);

  load_dir_button = gtk_button_new_with_label ("Load from Directory");
  g_signal_connect (load_dir_button, "clicked", G_CALLBACK (load_songs_cb), NULL);
  gtk_container_add (GTK_CONTAINER (load_dir_button_box), load_dir_button);

  gtk_grid_attach(GTK_GRID(grid), load_dir_button_box, 1, 0, 1, 1);

  //list box
  list_box = gtk_list_box_new();

  gtk_grid_attach(GTK_GRID(grid), list_box, 0, 1, 2, 1);

  gtk_widget_set_size_request(list_box, -1, 740);

  //set up search bar.
  search_bar = gtk_search_entry_new();
  //search_entry = gtk_search_entry_new();

  //gtk_search_bar_connect_entry(GTK_SEARCH_BAR(search_bar), GTK_ENTRY(search_entry));

  g_signal_connect(G_OBJECT(search_bar), "search-changed", G_CALLBACK(search_bar_cb), NULL);

  //gtk_container_add(GTK_CONTAINER(search_bar), search_entry);

  //gtk_widget_show(search_bar);

  //gtk_search_bar_connect_entry(GTK_SEARCH_BAR(search_bar), GTK_ENTRY(search_entry));

  gtk_grid_attach(GTK_GRID(grid), search_bar, 0, 2, 2, 1);

  //set up window_seperator.
  window_separator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);

  gtk_grid_attach(GTK_GRID(grid), window_separator, 3, 0, 1, 3);

  //set up the right grid.
  right_grid = gtk_grid_new();

  gtk_grid_set_row_spacing(GTK_GRID(right_grid), 10);
  gtk_grid_set_column_spacing(GTK_GRID(right_grid), 10);

  gtk_grid_attach(GTK_GRID(grid), right_grid, 4, 0, 1, 3);

  //top labels.
  info_label1 = gtk_label_new("Select a file from list to start ...");
  info_label2 = gtk_label_new("");
  info_label3 = gtk_label_new("");
  info_label4 = gtk_label_new("");


  gtk_label_set_xalign(GTK_LABEL(info_label1), 0.0);
  gtk_label_set_xalign(GTK_LABEL(info_label2), 0.0);
  gtk_label_set_xalign(GTK_LABEL(info_label3), 0.0);
  gtk_label_set_xalign(GTK_LABEL(info_label4), 0.0);


  gtk_grid_attach(GTK_GRID(right_grid), info_label1, 0, 0, 2, 1);
  gtk_grid_attach(GTK_GRID(right_grid), info_label2, 0, 1, 2, 1);
  gtk_grid_attach(GTK_GRID(right_grid), info_label3, 0, 3, 2, 1);
  gtk_grid_attach(GTK_GRID(right_grid), info_label4, 0, 4, 2, 1);

  //set up t_scale
  t_scale_label = gtk_label_new("T scale:");

  t_scale = gtk_spin_button_new(gtk_adjustment_new(0.0, 0.0, 100.0, 1.0, 5.0, 0.0), 1, 0);

  g_signal_connect(G_OBJECT(t_scale), "value-changed", G_CALLBACK(time_scale_cb), NULL);

  gtk_grid_attach(GTK_GRID(right_grid), t_scale_label, 0, 5, 1, 1);
  gtk_grid_attach(GTK_GRID(right_grid), t_scale, 1, 5, 1, 1);

  //set up original song and after effect.
  original_song_frame = gtk_frame_new("Original Song");

  original_draw_area = gtk_drawing_area_new();

  original_scroll = gtk_scrolled_window_new(gtk_adjustment_new(0.0, 0.0, 100.0, 1.0, 5.0, 0.0), NULL);

  gtk_container_add(GTK_CONTAINER(original_scroll), original_draw_area);

  after_effect_frame = gtk_frame_new("After Effect:");

  after_scroll = gtk_scrolled_window_new(gtk_adjustment_new(0.0, 0.0, 100.0, 1.0, 5.0, 0.0), NULL);

  after_draw_area = gtk_drawing_area_new();

  gtk_container_add(GTK_CONTAINER(after_scroll), after_draw_area);

  g_signal_connect(G_OBJECT(original_draw_area), "draw", G_CALLBACK(draw_cb), g_current_song);
  g_signal_connect(G_OBJECT(after_draw_area), "draw", G_CALLBACK(draw_cb), g_modified_song);

  //set the original size of the draw area to 600 x 200.
  gtk_widget_set_size_request(original_draw_area, 600, 200);
  gtk_widget_set_size_request(after_draw_area, 600, 200);

  //add the scroll windows to the frames.
  gtk_container_add(GTK_CONTAINER(original_song_frame), original_scroll);
  gtk_container_add(GTK_CONTAINER(after_effect_frame), after_scroll);

  //set the size of the scroll windows to 600 x 200.
  gtk_widget_set_size_request(original_scroll, 600, 200);
  gtk_widget_set_size_request(after_scroll, 600, 200);

  //add the frames to the window grid.
  gtk_grid_attach(GTK_GRID(right_grid), original_song_frame, 0, 6, 2, 1);
  gtk_grid_attach(GTK_GRID(right_grid), after_effect_frame, 0, 7, 2, 1);

  //bottom buttons
  //GtkAdjustment * warp_time_adjust = gtk_adjustment_new(0.0, -10.0, 10.0, 0.1, 5.0, 0.0);
  GtkAdjustment * change_octave_adjust = gtk_adjustment_new(0.0, -5.0, 5.0, 1.0, 5.0, 0.0);

  warp_time_button = gtk_spin_button_new_with_range(-10.0, 10.0, 0.1);
  warp_time_label = gtk_label_new("Warp Time: ");

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(warp_time_button), 1.0);

  g_signal_connect(G_OBJECT(warp_time_button), "value-changed", G_CALLBACK(warp_time_cb), NULL);

  change_octave_button = gtk_spin_button_new(change_octave_adjust, 1, 0);
  change_octave_label = gtk_label_new("Change Octave: ");

  g_signal_connect(G_OBJECT(change_octave_button), "value-changed", G_CALLBACK(song_octave_cb), NULL);

  remap_notes_list = gtk_combo_box_text_new();
  remap_notes_label = gtk_label_new("Remap Notes: ");

  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(remap_notes_list), NULL, "Lower");

  remap_instruments_list = gtk_combo_box_text_new();
  remap_instruments_label = gtk_label_new("Remap Instruments: ");

  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(remap_instruments_list), NULL, "Brass Band");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(remap_instruments_list), NULL, "Helicopter");


  g_signal_connect(G_OBJECT(remap_instruments_list), "changed", G_CALLBACK(instrument_map_cb), NULL);
  g_signal_connect(G_OBJECT(remap_notes_list), "changed", G_CALLBACK(note_map_cb), NULL);

  gtk_grid_attach(GTK_GRID(right_grid), warp_time_label, 0, 8, 1, 1);
  gtk_grid_attach(GTK_GRID(right_grid), warp_time_button, 1, 8, 1, 1);

  gtk_grid_attach(GTK_GRID(right_grid), change_octave_label, 0, 9, 1, 1);
  gtk_grid_attach(GTK_GRID(right_grid), change_octave_button, 1, 9, 1, 1);

  gtk_grid_attach(GTK_GRID(right_grid), remap_instruments_label, 0, 10, 1, 1);
  gtk_grid_attach(GTK_GRID(right_grid), remap_instruments_list, 1, 10, 1, 1);

  gtk_grid_attach(GTK_GRID(right_grid), remap_notes_label, 0, 11, 1, 1);
  gtk_grid_attach(GTK_GRID(right_grid), remap_notes_list, 1, 11, 1, 1);

   //very bottom buttons
  remove_song_button = gtk_button_new_with_label("Remove Song");
  remove_song_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);

  gtk_container_add(GTK_CONTAINER(remove_song_box), remove_song_button);

  save_song_button = gtk_button_new_with_label("Save Song");
  save_song_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);

  g_signal_connect(remove_song_button, "clicked", G_CALLBACK(remove_song_cb), NULL);
  g_signal_connect(save_song_button, "clicked", G_CALLBACK (save_song_cb), NULL);

  gtk_container_add(GTK_CONTAINER(save_song_box), save_song_button);

  gtk_grid_attach(GTK_GRID(right_grid), remove_song_box, 0, 12, 1, 1);
  gtk_grid_attach(GTK_GRID(right_grid), save_song_box, 1, 12, 1, 1);


  //show the window.
  gtk_widget_show_all(window);
}

/*
 *  add_song_cb
 */

void add_song_cb(GtkButton * button, gpointer ptr) {
  GtkWidget * dialog;

  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;

  gint res;

  dialog = gtk_file_chooser_dialog_new("Open file", ptr, action, "Cancel",
    GTK_RESPONSE_CANCEL, "Open", GTK_RESPONSE_ACCEPT, NULL);

  res = gtk_dialog_run (GTK_DIALOG (dialog));

  if (res == GTK_RESPONSE_ACCEPT) {
    char * filepath;
    GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
    filepath = gtk_file_chooser_get_filename (chooser);

    //printf("%s\n", filepath);

    if (strstr(filepath, ".mid") != NULL) {
      //allocate memory for the new node.
      tree_node_t * new_node = malloc(sizeof(tree_node_t));
      assert(new_node);

      //parse the file.
      new_node->song = parse_file(filepath);

      //find the length of the filename
      size_t filepath_len = strlen(filepath) + 1;

      char temp_filename[filepath_len];

      strncpy(temp_filename, filepath, filepath_len);

      //get the original position of the filename
      char * original_pos = temp_filename;

      //get the substrings of filename.
      char * token = strtok(temp_filename, "/");

      char * filename = token;

      while (token != NULL) {
        filename = token;
        token = strtok(NULL, "/");
      }

      //get the pointer to the beginning of the filename and not the filepath.
      int difference = filename - original_pos;

      //set the song_name to the pointer to the dynamically allocated string
      //allocated in parse_file. The pointer points to the beginning of the
      //filename not the whole filepath.
      new_node->song_name = new_node->song->path + difference;

      //set these to NULL as a protective programming practice.
      new_node->left_child = NULL;
      new_node->right_child = NULL;
/*
      if (g_song_library == NULL) {
        printf("Empty!\n");
      }
*/
      //insert the new node into the global song library and assert success.
      assert(tree_insert(&g_song_library, new_node) == INSERT_SUCCESS);
    } else {
      GtkWidget * error = gtk_message_dialog_new(ptr,
        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
        "%s is not a valid file.", filepath);

      gtk_dialog_run(GTK_DIALOG(error));
      gtk_widget_destroy(error);
    }

    g_free (filepath);
  }

  update_song_list();

  gtk_widget_destroy (dialog);
}

/*
 *  load_songs_cb
 */

void load_songs_cb(GtkButton * button, gpointer ptr) {
  GtkWidget * dialog;

  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;

  gint res;

  dialog = gtk_file_chooser_dialog_new("Load From Directory", ptr, action,
    "Cancel", GTK_RESPONSE_CANCEL, "Open", GTK_RESPONSE_ACCEPT, NULL);

  gtk_file_chooser_set_action(GTK_FILE_CHOOSER(dialog),
    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

  res = gtk_dialog_run (GTK_DIALOG (dialog));

  if (res == GTK_RESPONSE_ACCEPT) {
    gchar * directory;
    GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);

    directory = gtk_file_chooser_get_current_folder(chooser);

    //g_print("%s\n", directory);

    //TODO make a helper function for this so there is not an assertion fail
    //and instead a dialog appears.

    make_library(directory);

    update_song_list();

    g_free (directory);
  }

  gtk_widget_destroy (dialog);
}

/*
 *  song_selected_cb 
 */

void song_selected_cb(GtkListBox * list_box, GtkListBoxRow * row) {
  GtkWidget * label = gtk_bin_get_child(GTK_BIN(row));

  const char * text = (const char*) gtk_label_get_text(GTK_LABEL(label));

  tree_node_t ** root = &g_song_library;

  root = find_parent_pointer(root, text);

  g_current_node = *root;

  g_current_song = g_current_node->song;

  if (g_modified_song) {
    free_song(g_modified_song);
  }

  g_modified_song = parse_file(g_current_song->path);

  //set the modification widgets to their default settings
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(warp_time_button), 1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(change_octave_button), 0.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(t_scale), 0.0);

  //update the info above the drawing areas.
  //update_song();
  //g_print("%p\n", g_modified_song);

  update_info();

}

void find_search_elements(tree_node_t * root, tree_node_t ** list, int * count, const char * regex) {
  if (root == NULL) {
    return;
  }

  find_search_elements(root->left_child, list, count, regex);

  if (strstr(root->song_name, regex)) {
    //g_print("%d %s\n", *count, root->song_name);
    list[*count] = root;
    (*count)++;
  }

  find_search_elements(root->right_child, list, count, regex);
}

void number_of_tree_elements(tree_node_t * root, void * count) {
  if (root == NULL) {
    return;
  }

  number_of_tree_elements(root->left_child, count);

  int * num = (int*) count;

  (*num)++;

  number_of_tree_elements(root->right_child, count);
}


/* Define search_bar_cb here */

void search_bar_cb(GtkSearchBar * search_bar, gpointer ptr) {
  GtkEntry * entry = GTK_ENTRY(search_bar);

  const char * str = gtk_entry_get_text(entry);

  //traverse through the tree
  //g_print("%s\n", str);
  if (g_song_library == NULL) {
    return;
  }

  if (str[0] == '\0') {
    //g_print("we got them\n");
    update_song_list();
    return;
  }

  int elements = 0;

  number_of_tree_elements(g_song_library, (void*)(&elements));

  //g_print("total:%d\n", elements);

  tree_node_t * list[elements];

  int found_elements = 0;
  
  find_search_elements(g_song_library, list, &found_elements, str);

  //g_print("found: %d\n", found_elements);

  //destroy the old list_box
  gtk_widget_destroy(list_box);

  //make a new list_box.
  list_box = gtk_list_box_new();

  //attach an event listener to call the song_selected_cb.
  g_signal_connect(list_box, "row-activated", G_CALLBACK(song_selected_cb), 
    NULL);

  //attach the list_box in the correct grid area.
  gtk_grid_attach(GTK_GRID(grid), list_box, 0, 1, 2, 1);

  //size the new list_box at 700 pixels height.
  gtk_widget_set_size_request(list_box, -1, 740);

  //set the number of elements in the list to 0.
  g_list_box_elements = 0;

  for (int i = 0; i < found_elements; i++) {
    if (list[i] == NULL) {
      break;
    }

    //g_print("%d %s\n", i, list[i]->song_name);

    GtkWidget * list_box_row = gtk_list_box_row_new();
    GtkWidget * list_label = gtk_label_new(list[i]->song_name);

    gtk_label_set_xalign(GTK_LABEL(list_label), 0.0);

    gtk_container_add(GTK_CONTAINER(list_box_row), list_label);

    gtk_list_box_insert(GTK_LIST_BOX(list_box), list_box_row, i);
  }


  //show the newly made list.
  gtk_widget_show_all(grid);

}

/*
 *  time_scale_cb
 */

void time_scale_cb(GtkSpinButton * spin_button, gpointer ptr) { 
  gint val = gtk_spin_button_get_value_as_int(spin_button);

  //g_print("%d\n", val);
  note_width = val;

  update_drawing_area();
}

/*
 *  draw_cb
 */

gboolean draw_cb(GtkDrawingArea * draw_area, cairo_t * cairo, gpointer ptr) {
  song_data_t * song = NULL;
if (ptr != g_current_song && ptr != g_modified_song) {
    //g_print("found error\n");
    return FALSE;
  }
/*
  if (ptr == NULL) {
    g_print("gotem\n");
    return FALSE;
  }
*/


  guint width = gtk_widget_get_allocated_width(GTK_WIDGET(draw_area));
  guint height = gtk_widget_get_allocated_height(GTK_WIDGET(draw_area));

  GdkRGBA background_color;

  gdk_rgba_parse(&background_color, "#FFFFFF");

  gdk_cairo_set_source_rgba (cairo, &background_color);

  cairo_rectangle(cairo, 0, 0, width, height);

  cairo_fill(cairo);

  if (ptr == NULL) {
    return FALSE;
  }

  if (ptr == g_current_song) {
    //g_print("Current_Song\n");
  }

  if (ptr == g_modified_song) {
    //g_print("Modified Song\n");
  }
/*
  if (ptr != g_current_song && ptr != g_modified_song) {
    g_print("found error\n");
    return FALSE;
  }
*/
  ///g_print("Draw\n");

  song = ptr;

  int highest_note = 0;
  int lowest_note = 0;
  int song_length = 0;

  range_of_song(song, &lowest_note, &highest_note, &song_length);

  if (note_width != 0) {
    gtk_widget_set_size_request(GTK_WIDGET(draw_area), (song_length * 1.0) / note_width, 200);
  } else {
    gtk_widget_set_size_request(GTK_WIDGET(draw_area), 600, 200);
  }

  width = gtk_widget_get_allocated_width(GTK_WIDGET(draw_area));
  height = gtk_widget_get_allocated_height(GTK_WIDGET(draw_area));

  int note_diff = highest_note - lowest_note;
  double note_inc = (height * 1.0) / note_diff;

  //g_print("Height: %d, Note_Diff: %d, Note_Inc: %f\n", height, note_diff, note_inc);

  double time_inc = (width * 1.0) / song_length;

  //make the middle C line.
  cairo_set_source_rgb(cairo, 0, 0, 0);
  cairo_set_line_width(cairo, 0.5);

  //by midi definition.
  uint8_t middle_c = 60;

  //g_print("middle_c: %f\n", height - ((middle_c - lowest_note) * note_inc));

  cairo_move_to(cairo, 0, height - ((middle_c - lowest_note) * note_inc));
  cairo_line_to(cairo, width, height - ((middle_c - lowest_note) * note_inc));

  cairo_stroke(cairo);

  //paint each note in the song.
  track_node_t * current_track = song->track_list;
  event_node_t * current_event = NULL;

  gdk_cairo_set_source_rgba (cairo, COLOR_PALETTE[0]);

  uint8_t current_instrument = 0;

  while (current_track) {
    current_event = current_track->track->event_list;
    int current_posx = 0;
    while (current_event) {
      current_posx += current_event->event->delta_time;

      if ((current_event->event->type >= 0xC0) &&
          (current_event->event->type <= 0xCF)) {
        current_instrument = current_event->event->midi_event.data[0];
        gdk_cairo_set_source_rgba(cairo, COLOR_PALETTE[current_instrument]);
        current_event = current_event->next_event;
        continue;
      }

      //note on event
      if ((current_event->event->type >= 0x90) &&
          (current_event->event->type <= 0x9F)) {
        event_node_t * find_note_end = current_event;

        int note_off_time = current_posx;

        uint8_t channel = 0x0F & current_event->event->type;

        int note = current_event->event->midi_event.data[0];

        while (find_note_end) {
          note_off_time += find_note_end->event->delta_time;

          //a note off event.
          if (find_note_end->event->type == (0x80 | channel)) {
            if (find_note_end->event->midi_event.data[0] == note) {
              break;
            }
          }

          //a note with velocity 0.
          if (find_note_end->event->type == (0x90 | channel)) {
            if (find_note_end->event->midi_event.data[0] == note) {
              if (find_note_end->event->midi_event.data[1] == 0) {
                break;
              }
            }
          }

          find_note_end = find_note_end->next_event;
        }

        int time_diff = note_off_time - current_posx;

        double x = 0.0;

        if (note_width == 0) {
          x = current_posx * time_inc;
        } else {
          x = (current_posx * 1.0) / note_width;
        }

        double y = height - ((note - lowest_note) * note_inc);


        double shape_width = 0.0;

        if (note_width == 0) {
          shape_width = time_diff * time_inc;
        } else {
          shape_width = (time_diff * 1.0) / note_width;
        }
        double shape_height = note_inc;

        cairo_rectangle(cairo, x, y, shape_width, shape_height);

        cairo_fill(cairo);

      }
      current_event = current_event->next_event;
    }
    current_track = current_track->next_track;
  }

  return FALSE;
}

/*
 *  warp_time_cb
 */

void warp_time_cb(GtkSpinButton * spin_button, gpointer ptr) {
  gdouble val = gtk_spin_button_get_value(spin_button);

  time_warp = val;

  update_song();
}

/*
 *  song_octave_cb
 */

void song_octave_cb(GtkSpinButton * spin_button, gpointer ptr) {
  gint val = gtk_spin_button_get_value(spin_button);
  //g_print("song_octave\n");
  octave_change = val;

  update_song();
}

/*
 *  instrument_map_cb
 */

void instrument_map_cb(GtkComboBoxText * text, gpointer ptr) {
  char * active_str = gtk_combo_box_text_get_active_text(text);

  if (strcmp(active_str, "Helicopter") == 0) {
    remap_instruments(g_modified_song, I_HELICOPTER);
  } else if (strcmp(active_str, "Brass Band") == 0) {
    remap_instruments(g_modified_song, I_BRASS_BAND);
  } else {
    //g_print("error?");
  }

  //update_song();
  update_drawing_area();
}

/*
 *  note_map_cb
 */

void note_map_cb(GtkComboBoxText * text, gpointer ptr) {
  char * active_str = gtk_combo_box_text_get_active_text(text);

  if (strcmp(active_str, "Lower") == 0) {
    remap_notes(g_modified_song, N_LOWER);
  } else {
    //g_print("error? notes");
  }

  //update_song();
  update_drawing_area();
}

/*
 *  save_song_cb
 */

void save_song_cb(GtkButton * button, gpointer ptr) {
  int diff = g_current_node->song_name - g_current_song->path;

  g_current_node->song = g_modified_song;

  g_current_node->song_name = g_modified_song->path + diff;

  free_song(g_current_song);

  g_current_song = NULL;
  g_modified_song = NULL;

  write_song_data(g_current_node->song, g_current_node->song->path);

  //g_print("name: %s\n", g_current_node->song_name);

  g_current_node = NULL;

  update_info();
  update_drawing_area();
  update_song_list();
  //g_print("test");
  /*
  GtkWidget *dialog;
  GtkFileChooser *chooser;
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
  gint res;

  dialog = gtk_file_chooser_dialog_new ("Save File",
                                        ptr,
                                        action,
                                        "Cancel",
                                        GTK_RESPONSE_CANCEL,
                                        "Save",
                                        GTK_RESPONSE_ACCEPT,
                                        NULL);
  chooser = GTK_FILE_CHOOSER (dialog);

  gtk_file_chooser_set_do_overwrite_confirmation (chooser, TRUE);
  gtk_file_chooser_set_filename (chooser, "New Song");

  res = gtk_dialog_run (GTK_DIALOG (dialog));
  if (res == GTK_RESPONSE_ACCEPT) {
    char *filename;

    filename = gtk_file_chooser_get_filename (chooser);
    //save_to_file (filename);i

    


    g_free (filename);
  }

  gtk_widget_destroy (dialog);
  */
}

/*
 *  remove_song_cb
 */

void remove_song_cb(GtkButton * button, gpointer ptr) {
  remove_song_from_tree(&g_song_library, g_current_node->song_name);
  g_current_node = NULL;
  g_current_song = NULL;
  free_song(g_modified_song);
  g_modified_song = NULL;
  update_song_list();
  update_info();
  update_drawing_area();
}

/*
 * Function called prior to main that sets up the instrument to color mapping
 */

void build_color_palette()
{
  static GdkRGBA palette[16];

  memset(COLOR_PALETTE, 0, sizeof(COLOR_PALETTE));
  char* color_specs[] = {
    // Piano, red
    "#ff0000",
    // Chromatic percussion, brown
    "#8b4513",
    // Organ, purple
    "#800080",
    // Guitar, green
    "#00ff00",
    // Bass, blue
    "#0000ff",
    // Strings, cyan
    "#00ffff",
    // Ensemble, teal
    "#008080",
    // Brass, orange
    "#ffa500",
    // Reed, magenta
    "#ff00ff",
    // Pipe, yellow
    "ffff00",
    // Synth lead, indigo
    "#4b0082",
    // Synth pad, dark slate grar
    "#2f4f4f",
    // Synth effects, silver
    "#c0c0c0",
    // Ehtnic, olive
    "#808000",
    // Percussive, silver
    "#c0c0c0",
    // Sound effects, gray
    "#808080",
  };

  for (int i = 0; i < 16; ++i) {
    gdk_rgba_parse(&palette[i], color_specs[i]);
    for (int j = 0; j < 8; ++j) {
      COLOR_PALETTE[i * 8 + j] = &palette[i];
    }
  }
} /* build_color_palette() */
