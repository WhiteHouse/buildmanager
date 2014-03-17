<?php
/**
 * @file
 * buildmanager drush command.
 *
 *  You can copy this file to any of the following
 *    1. A .drush folder in your HOME folder.
 *    2. Anywhere in a folder tree below an active module on your site.
 *    3. /usr/share/drush/commands (configurable)
 *    4. In an arbitrary folder specified with the --include option.
 *    5. Drupal's /drush or /sites/all/drush folders.
 */

/**
 * Implements hook_drush_command().
 */
function buildmanager_drush_command() {

  $items = array();

  $items['buildmanager-build'] = array(
    'description' => "Wrapper around drush make for (re)building site repos.",
    'arguments' => array(
      'config' => '(Optional) path/to/buildmanager.config.yml',
    ),
    'options' => array(
      'show-info' => array(
        'description' => 'Print $info array from combined make files. (Helpful for development.)',
      ),
      'show-config' => array(
        'description' => 'Print $config array from mysite.config.inc. (Helpful for development.)',
      ),
      'no-commit' => array(
        'description' => 'Do not commit changes. Note: To skip subtree add, pull, or merge use no-subtree-updates.',
      ),
      'no-subtree-updates' => array(
        'description' => 'Do not commit changes. NOTE: ',
      ),
      'message' => array(
        'description' => 'Message to include at the beginning of any new commits.',
      ),
      'simulate' => array(
        'description' => 'Output commands to be executed for examination, but do not actually execute them.',
      ),
      // @todo
      're-add' => array(
        'description' => '@todo Remove and re-add comma separated list of subtree(s).',
      ),
    ),
    'examples' => array(
      'drush buildmanager-build' => '',
      'drush bmb' => '',
      'drush bmb --show-info' => '',
      'drush bmb --message="Rebuild with example_distro release 7.x-1.5"' => '',
      'drush bmb ./buildmanager.config.inc --re-add=example1,example2,exampleN' => '',
    ),
    'aliases' => array('bmb'),
    // No bootstrap at all.
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );

  return $items;
}

/**
 * Callback for buildmanager-build command.
 *
 * @param string $config_file
 *   path/to/config/file providing $config info
 *
 * @see drush_invoke()
 * @see drush.api.php
 */
function drush_buildmanager_build($config_file = '') {
  // Build up array of commands to be executed in order.
  $commands = array();

  // Get config file.
  if (!$config_file) {
    $config_file = buildmanager_get_config_file();
  }
  drush_log(dt('Loading configuration from: !config_file', array('!config_file' => $config_file)), 'ok');
  $config = _buildmanager_get_config($config_file);

  // Assemble $info array by loading, parsing and merging make file(s).
  $build_file = $config['build_file'];
  drush_log(dt('Using build file: !build_file', array('!build_file' => $build_file)), 'ok');
  $info = _buildmanager_get_info($build_file);

  // If requested, output full build info.
  if (drush_get_option('show-info')) {
    drush_print('$info:');
    drush_print_r($info);
  }

  // Add/update git subtrees.
  // @todo Check if there are uncommitted changes locally. Exit if there are.
  // Otherwise, subtree add/pull won't work.
  // If there are subtrees in the config file, get commands for adding/updating
  // subtrees.
  if (isset($config['subtrees'])) {

    drush_log(dt('Preparing to add/update git subtrees specified here: !config_file', array('!config_file' => $config_file)), 'ok');

    // Prep projects directory. All subtrees will use --prefix=projects/$name
    _buildmanager_prep_projects_directory();

    // Hang onto symlink commands here. Add these to the $commands array after
    // drush make.
    $symlink_commands = array();

    foreach ($config['subtrees'] as $subtree_name => $details) {
      // Get project info from make file.
      $project_info = (isset($info['projects'][$subtree_name])) ? $info['projects'][$subtree_name] : array();
      // Get subtree commands.
      $subtree_commands = array();
      if (!drush_get_option('no-subtree-updates', FALSE)) {
        $subtree_commands = _drushsubtree_get_subtree_commands($subtree_name, $details, $project_info);
      }
      else {
        drush_log(dt('Skipping git subtree updates.'), 'ok');
      }
      // Remove files checked out by drush. Add symlink pointing to your local
      // subtree.
      drush_log(dt('Preparing to symlink !subtree_name to path: !path', array('!subtree_name' => $subtree_name, '!path' => $details['path'])), 'ok');
      $symlink_commands[] = _drushsubtree_subtree_add_symlink($subtree_name, $details['path']);
      // Add subtree commands to the beginning of the $commands array, to be
      // executed before drush make.
      $commands = array_merge($commands, $subtree_commands);
    }

  }

  // Execute subtree commands now. This way, drush make will include info
  // from make files updated with git subtree pulls.
  foreach ($commands as $command) {
    // @todo Add error handling.
    drush_shell_exec($command);
  }
  // Zero out $commands. Build up next array of commands to be executed.
  $commands = array();

  // Drush make.
  // If target director already exists, remove it.
  $target = $config['target'];
  if (file_exists($target)) {
    $commands[] = "rm -rf $target";
  }
  $commands[] = DRUSH_COMMAND . " make {$build_file} {$target} --no-recursion --no-gitinfofile";

  // Replace downloaded projects with symlinks where subtrees are used.
  $commands = array_merge($commands, $symlink_commands);

  // Custom, site-specific commands defined by config file.
  // Add custom commands from site make config file.
  foreach ($config['commands'] as $command) {
    $commands[] = $command;
  }

  // Commit.
  if (!drush_get_option('no-commit', FALSE)) {
    $message = ($message = drush_get_option('message', FALSE)) ? $message : dt('Rebuild with drush buildmanager.');
    $commands[] = _buildmanager_commit($message);
  }

  // Execute commands.
  foreach ($commands as $command) {
    drush_shell_exec($command);
  }

}
/**
 * Implements drush_hook_COMMAND_validate().
 */
function drush_buildmanager_build_validate() {
  $is_valid = TRUE;

  // TODO Check drush version. Currently, this requires master branch with
  // --no-recurse option.
  // TODO Check ownership. If ant rebuilds, ownership may be funny.
  // TODO This command must be run from top-level of git repo.
  // TODO Confirm we're in a git repo. cd "$(git rev-parse --show-toplevel)"
  // TODO Confirm config points to a make file.
  // TODO Check code base for make files. See if they're all being included in
  // build. If not, notify user. Ask if we should proceed or not.
  // Helper stuff.
  // TODO Check for subtrees. Give user message if not using.
  // TODO Check for commands to shell_exec. Give user message if not using.
  // Confirm we have a config file.

  return $is_valid;
}

/**
 * Scan for config file. Prompt for selection if more than one found.
 *
 * @return string
 *   File name.
 */
function buildmanager_get_config_file() {
  $config_files = array();

  // Scan directory for config files like buildmanager.*.
  exec('pwd', $output, $exit_code);
  $directory = $output[0];
  foreach (drush_scan_directory($directory, '/.*/', array(), 0, FALSE) as $file) {
    $len = strlen('buildmanager');
    if (substr($file->basename, 0, $len) == 'buildmanager') {
      $config_files[$file->basename] = $file->basename;
    }
  }

  if (count($config_files) < 1) {
    // No config found. Error.
    drush_return_error(dt('No config file found. See buildmanager/README.md.'));
  }
  elseif (count($config_files) > 1) {
    // If more than one found, prompt for input.
    $config_file = drush_choice($config_files, dt('Select a config file to use:'));
  }
  else {
    // Only one config file fount.
    $config_file = array_shift($config_files);
  }

  return $config_file;
}

/**
 * Load config from config file.
 *
 * Helpful tip for developers: You can convert a PHP array to YAML via drush
 * like this. If you know the $config array you want, but you're not sure how to
 * form at it, build the array, then run it through this function:
 *
 *   drush_print(drush_format($config, NULL, 'yaml'));
 *
 * @return array
 *   Return parsed YAML config as PHP array.
 */
function _buildmanager_get_config($config_file) {
  if (!class_exists('Symfony\Component\Yaml\Parser')) {
    // For Drush PSR-0 and Composer information, see:
    // http://drupal.org/node/1316322.
    drush_log('Autoloading Symfony\Component\Yaml\Parser failed.', 'error');
    return;
  }
  $parser = new Symfony\Component\Yaml\Parser();
  $config = $parser->parse(file_get_contents($config_file));

  // If requested, output loaded config.
  if (drush_get_option('show-config', FALSE)) {
    drush_print('$config:');
    drush_print(drush_format($config, NULL, 'yaml'));
  }

  return $config;
}

/**
 * Parse make/info files.
 *
 * @return array
 *   Assemble $info array by loading, parsing and merging make file(s).
 */
function _buildmanager_get_info($file) {
  // Load $info from info/make file if it exists.
  if (file_exists($file)) {
    $info = make_parse_info_file($file);
  }
  else {
    return drush_set_error('buildmanager', dt('Make file does not exist: !here.', array('!here' => $file)));
  }

  // Check for included make files. If they exist, recurse.
  $merge_info = array();
  if (isset($info['includes'])) {
    foreach ($info['includes'] as $include) {
      $more_info = _buildmanager_get_info($include);
      $merge_info = array_merge_recursive_distinct($merge_info, $more_info);
    }
  }

  $info = array_merge_recursive_distinct($info, $merge_info);

  return $info;
}

/**
 * Provides recursive array merge.
 *
 * For more info see http://danielsmedegaardbuus.dk/
 * 2009-03-19/phps-array_merge_recursive-as-it-should-be
 */
function &array_merge_recursive_distinct(array &$array1, &$array2 = NULL) {
  $merged = $array1;

  if (is_array($array2)) {
    foreach ($array2 as $key => $val) {
      if (is_array($array2[$key])) {
        $merged[$key] = is_array($merged[$key]) ? array_merge_recursive_distinct($merged[$key], $array2[$key]) : $array2[$key];
      }
      else {
        $merged[$key] = $val;
      }
    }
  }

  return $merged;
}

/**
 * Prep projects directory.
 */
function _buildmanager_prep_projects_directory() {
  // Before adding/pulling subtrees, make sure parent directory
  // for --prefix exists.
  if (!file_exists('projects')) {
    $success = mkdir('projects', 0777, TRUE);
    if (!$success) {
      return drush_set_error('buildmanager', dt("Failed creating directory: projects"), 'error');
    }
  }
  elseif (file_exists('projects') && !is_dir('projects')) {
    return drush_set_error('buildmanager', dt("Please remove the 'projects' file. We need to create a directory with that name for your subtrees."), 'error');
  }

  if ($success) {
    drush_log(dt('A projects directory has been created at the top of your git repo. Git subtrees will be added there.'), 'ok');
  }
}

/**
 * Commit all additions/deletions.
 *
 * @param string $message
 *   Commit message.
 *
 * @return string
 *   Commit command.
 */
function _buildmanager_commit($message) {
  if (drush_get_option('no-commit')) {
    drush_log(dt('Skipping git commit.'), 'ok');
    return '';
  }
  // Add any new files added to the repo.
  $command = "git add . ; git commit -am '{$message}';";
  return $command;
}
