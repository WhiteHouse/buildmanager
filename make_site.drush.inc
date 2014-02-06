<?php

/**
 * @file
 *   make-site drush command.
 *
 *   You can copy this file to any of the following
 *     1. A .drush folder in your HOME folder.
 *     2. Anywhere in a folder tree below an active module on your site.
 *     3. /usr/share/drush/commands (configurable)
 *     4. In an arbitrary folder specified with the --include option.
 *     5. Drupal's /drush or /sites/all/drush folders.
 */

/**
 * Implementation of hook_drush_command().
 */
function make_site_drush_command() {
  $items = array();

  $items['make-site'] = array(
    'description' => "Wrapper around drush make for (re)building site repos.",
    'arguments' => array(
      'config' => 'path/to/make-site.config.inc',
    ),
    /*
    'options' => array(
      'key' => array(
        'description' => '',
        'example-value' => '',
      ),
    ),
    // */
    'examples' => array(
      'drush make-site mysite.config.inc' => '',
    ),
    'aliases' => array('mks'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
  );

  return $items;
}

/**
 * Implementation of hook_drush_help().
 *
 * @param
 *   A string with the help section (prepend with 'drush:')
 *
 * @return
 *   A string with the help text for your command.
 */
function make_site_drush_help($section) {
  switch ($section) {
    case 'drush:make-site':
      return dt(""); // TODO
  }
}

/**
 * Implementation of drush_hook_COMMAND_validate().
 */
function drush_make_site_validate() {
  $is_valid = TRUE;

  // TODO Confirm we're in a git repo.
  // TODO Confirm config points to a make file.
  // TODO Check for subtrees. Give user message if not using.
  // TODO Check for commands to shell_exec. Give user message if not using.

  if (!$is_valid) {
    // TODO Improve error message here.
    return drush_set_error('make_site', dt('Validation failed.'));
  }
}

/**
 * Callback for make-site command.
 *
 * @see drush_invoke()
 * @see drush.api.php
 */
function drush_make_site() {
  // TODO Load config.
  // TODO Load and parse make file(s).
  // TODO Drush make.
  // TODO Set up subtrees.
  // TODO Execute custom commands.
}
