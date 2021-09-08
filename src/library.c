/* Nic Ballesteros, library.c, CS 24000, Fall 2020
 * Last updated November 8, 2020
 */

/* Add any includes here */
#include "library.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <ftw.h>


tree_node_t * g_song_library = NULL;

/*
 *  find_parent_pointer iterates through a binary tree and finds the element
 *  where the song_name of that element matches the song_name passed in as
 *  an argument.
 */

tree_node_t ** find_parent_pointer(tree_node_t ** root,
  const char * song_name) {
  //save the position the function is in the tree.
  tree_node_t * current_root = (*root);
  tree_node_t * current_child = (*root);

  //compare the strings song_name and the song_name in the current root.
  int compare_value = strcmp(song_name, current_root->song_name);

  //while the strings do not match,
  while (compare_value != 0) {
    //iterate through the tree until the correct element is found.
    if (compare_value < 0) {
      //string one is less than string two
      if (current_child->left_child) {
        current_root = current_child;
        current_child = current_child->left_child;
      } else {
        return NULL;
      }
    } else {
      //string two is less than string one
      if (current_child->right_child) {
        current_root = current_child;
        current_child = current_child->right_child;
      } else {
        return NULL;
      }
    }

    //recompare the strings with a new child.
    compare_value = strcmp(song_name, current_child->song_name);
  }

  //compare the song name and the song name of the current child.
  if (strcmp(song_name, current_child->song_name) == 0) {
    //the root of the found element is the current child.
    *root = current_child;

    return root;
  }

  return NULL;
} /* find_parent_pointer() */

/*
 *  tree_insert takes the root of a binary tree and a newly created child and
 *  finds where the newly created child should be inserted. If the song already
 *  exists, it returns DUPLICATE_SONG and on success it returns INSERT_SUCCESS.
 */

int tree_insert(tree_node_t ** root, tree_node_t * new_child) {
  //if root is NULL, new_child is the new root.
  if ((*root) == NULL) {
    *root = new_child;
    return INSERT_SUCCESS;
  }

  //printf("g_lib: %s\n", g_song_library->song_name);

  //keep track of the positions in the tree.
  tree_node_t * current_root = *root;
  tree_node_t * current_child = *root;

  //save the compare value between the two song names.
  int compare_value = strcmp(current_root->song_name, new_child->song_name);

  //iterate through the tree until the correct position for the new element is
  //found.
  while (1) {
    //if the two strings equal each other, there is a duplicate song.
    if (compare_value == 0) {
      return DUPLICATE_SONG;
    }

    //iterate through the tree
    if (compare_value > 0) {
      current_child = current_root->left_child;
    } else {
      current_child = current_root->right_child;
    }

    //if there is not a child under the current root, add new_child to it.
    if (current_child) {
      current_root = current_child;
    } else {
      if (compare_value > 0) {
        current_root->left_child = new_child;
      } else {
        current_root->right_child = new_child;
      }

      break;
    }

    //recompare the two strings.
    compare_value = strcmp(current_root->song_name, new_child->song_name);
  }

  //there were no errors so return success.
  return INSERT_SUCCESS;
} /* tree_insert() */

/*
 *  number_of_tree_elements takes a root of a binary tree and returns how many
 *  elements stem from that root.
 */

unsigned int number_of_tree_elements(tree_node_t * root) {
  //if root does not exist, return 0.
  if (!root) {
    return 0;
  }

  unsigned int count = 1;

  //count the number of elements.
  count += number_of_tree_elements(root->left_child);

  count += number_of_tree_elements(root->right_child);

  return count;
} /* number_of_tree_elements() */

/*
 *  store_pointers takes in a root and an array of tree_node_t pointers and
 *  populates the array with the elements of the tree under root. The third
 *  argument keeps track of the position of the array that needs to be filled.
 */

void store_pointers(tree_node_t * root, tree_node_t ** elements, int * index) {
  //if the root is NULL, return.
  if (root == NULL) {
    return;
  }

  //insert the left_child tree into elements.
  store_pointers(root->left_child, elements, index);

  //insert root into elements.
  elements[(*index)++] = root;

  //insert the right_child tree into elements.
  store_pointers(root->right_child, elements, index);

  //set the left and right children to NULL because they can get tangled if not.
  root->left_child = NULL;
  root->right_child = NULL;
} /* store_pointers() */

/*
 *  remove_song_from_tree removes a song from the tree. The function iterates
 *  through the tree and finds the element with the same song_name passed in
 *  as an argument, and removes that element from the tree. If that element of
 *  the tree has children, it reinserts those elements into the tree.
 */

int remove_song_from_tree(tree_node_t ** root, const char * song_name) {
  //save the position in the tree.
  tree_node_t * current_root = *root;
  tree_node_t * parent = *root;

  //compare the string in the tree and the string passed as an argument.
  int compare_value = strcmp(current_root->song_name, song_name);

  while (1) {
    if (compare_value == 0) {
      //the root with the given song_name has been found.
      break;
    } else if (compare_value > 0) {
      //go to the left child.
      if (current_root->left_child) {
        parent = current_root;
        current_root = current_root->left_child;
      } else {
        //the song was not found
        return SONG_NOT_FOUND;
      }
    } else {
      //go to the right child.
      if (current_root->right_child) {
        parent = current_root;
        current_root = current_root->right_child;
      } else {
        //the song was not found.
        return SONG_NOT_FOUND;
      }
    }

    //recompare the string within the new root and the argument string.
    compare_value = strcmp(current_root->song_name, song_name);
  }

  //save the left and right trees of the root that will be removed.
  tree_node_t * left_child = current_root->left_child;
  tree_node_t * right_child = current_root->right_child;

  //detach the element from the list.
  if (parent->right_child == current_root) {
    parent->right_child = NULL;
  }

  if (parent->left_child == current_root) {
    parent->left_child = NULL;
  }

  int number_of_children = number_of_tree_elements(left_child) +
                           number_of_tree_elements(right_child);

  //if the root has children,
  if (number_of_children > 0) {
    //store all the addresses of the children of the node that is being removed.
    tree_node_t * elements[number_of_children];

    //save the index of elements.
    int index = 0;

    //store the pointers of the left and right trees into elements.
    store_pointers(left_child, elements, &index);
    store_pointers(right_child, elements, &index);

    //if the found root is the root of the entire tree, use the left_child as
    //the new root of the entire tree.
    if (current_root == *root) {
      *root = left_child;
    }

    //reinsert the children into the tree.
    for (int i = 0; i < number_of_children; i++) {
      tree_insert(root, elements[i]);
    }

  }

  //free the removed node.
  free_node(current_root);

  //set it to NULL as a protective programming practice.
  current_root = NULL;

  return DELETE_SUCCESS;
} /* remove_song_from_tree() */

/*
 *  free_node deallocates all memory associated with a tree_node_t. It also sets
 *  the left and right children to NULL so that the tree does not accidentally
 *  get tangled. The function uses free_song defined in parser.c to free all
 *  the song data of the node.
 */

void free_node(tree_node_t * node) {
  //assert the node exists.
  assert(node);

  //set the right and left children to NULL.
  node->left_child = NULL;
  node->right_child = NULL;

  //free the song data in the node.
  free_song(node->song);

  //set the song pointer to NULL.
  node->song = NULL;

  //free the tree_node_t.
  free(node);

  //set it to NULL.
  node = NULL;
} /* free_node() */

/*
 *  print_node outputs the name of an song element in the tree pointed to by
 *  node, followed by a newline character, into a file.
 */

void print_node(tree_node_t * node, FILE * file) {
  //asssert that the file has been opened.
  assert(file);

  //get the length of the string and add two for the NUL and Newline bytes.
  size_t name_len = strlen(node->song_name) + 2;

  //make a new string.
  char new_string[name_len];

  //copy the name string to a new string.
  strcpy(new_string, node->song_name);

  //NUL terminate and add a newline.
  new_string[name_len - 2] = '\n';
  new_string[name_len - 1] = '\0';

  //printf("new_string: %s\n", new_string);

  //put the new string in the file.
  fputs(new_string, file);
} /* print_node() */

/*
 *  traverse_pre_order traverses through the tree pointed to by node in the
 *  preorder form and in this order preforms traverse, a function pointer, on
 *  each element. Data can be added to each element in the tree, in this order,
 *  using traverse.
 */

void traverse_pre_order(tree_node_t * node, void * data,
  traversal_func_t traverse) {
  //check for NULL arg.
  if (node == NULL) {
    return;
  }

  //traverse root.
  traverse(node, data);

  //traverse left child.
  if (node->left_child) {
    traverse_pre_order(node->left_child, data, traverse);
  }

  //traverse right child.
  if (node->right_child) {
    traverse_pre_order(node->right_child, data, traverse);
  }
} /* traverse_pre_order() */

/*
 *  traverse_in_order traverses through the tree pointed to by node in the
 *  in order form and in this order preforms traverse, a function pointer, on
 *  each element. Data can be added to each element in the tree, in this order,
 *  using traverse.
 */

void traverse_in_order(tree_node_t * node, void * data,
  traversal_func_t traverse) {
  //check for NULL arg.
  if (node == NULL) {
    return;
  }

  //traverse left child.
  if (node->left_child) {
    traverse_in_order(node->left_child, data, traverse);
  }

  //taverse the root.
  traverse(node, data);

  //traverse right child.
  if (node->right_child) {
    traverse_in_order(node->right_child, data, traverse);
  }

} /* traverse_in_order() */

/*
 *  traverse_post_order traverses through the tree pointed to by node in the
 *  postorder form and in this order preforms traverse, a function pointer, on
 *  each element. Data can be added to each element in the tree, in this order,
 *  using traverse.
 */

void traverse_post_order(tree_node_t * node, void * data,
  traversal_func_t traverse) {
  //check for NULL args.
  if (node == NULL) {
    return;
  }

  //traverse left child.
  if (node->left_child) {
    traverse_post_order(node->left_child, data, traverse);
  }

  //traverse right child.
  if (node->right_child) {
    traverse_post_order(node->right_child, data, traverse);
  }

  //traverse root.
  traverse(node, data);

} /* traverse_post_order() */

/*
 *  free_library frees all the memory associated with a song tree.
 */

void free_library(tree_node_t * node) {
  //if node does not exist, return.
  if (node == NULL) {
    return;
  }

  //free the left children nodes.
  free_library(node->left_child);

  //free the right children nodes.
  free_library(node->right_child);

  //free the node pointed to by node.
  free_node(node);
} /* free_library() */

/*
 *  write_song_list writes an entire tree to a file in a in order form.
 */

void write_song_list(FILE * file, tree_node_t * root) {
  //assert the file has been opened.
  assert(file);

  //if root does not exist, return.
  if (!root) {
    return;
  }

  //write the left children of the root to the file.
  write_song_list(file, root->left_child);

  //write the root to the file.
  print_node(root, file);

  //write the right children to the root to the file.
  write_song_list(file, root->right_child);
} /* write_song_list() */

/*
 *  create_node takes a filename where midi data is stored and inserts it into
 *  the global midi song tree.
 */

int create_node(const char * filepath, const struct stat * sb, int type) {
  //if the opened object is a file.
  if (type == FTW_NS) {
    return 0;
  }

  if (type == FTW_F) {
    //if there is a substring of ".mid".
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

      //insert the new node into the global song library and assert success.
      assert(tree_insert(&g_song_library, new_node) == INSERT_SUCCESS);
    }
  } else {
    return 0;
  }
  return 0;
} /* create_node() */

/*
 *  make_library reads in all the files in a directory and if they are a .mid
 *  file, parses them and puts them into g_song_library.
 */

void make_library(const char * directory) {
  ftw(directory, &create_node, 5);
} /* make_library() */
