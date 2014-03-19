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
      'message' => array(
        'description' => 'Message to include at the beginning of any new commits.',
      ),
      'simulate' => array(
        'description' => 'Output commands to be executed for examination, but do not actually execute them.',
      ),
    ),
    'examples' => array(
      'drush buildmanager-build' => '',
      'drush bmb' => '',
      'drush bmb --show-info' => '',
      'drush bmb --message="Rebuild with example_distro release 7.x-1.5"' => '',
    ),
    'aliases' => array('bmb'),
    // No bootstrap at all.
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );

  // Add options provided by Drush extensions extending buildmanager.
  $build_options = drush_command_invoke_all('buildmanager_build_options');
  $items['buildmanager-build']['options'] = array_merge(
    $items['buildmanager-build']['options'],
    $build_options
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
  // Build up arrays of prebuild and postbuild commands to be executed in order
  // before and after running `drush make`.
  $commands = new stdClass();
  $commands->prebuild = array();
  $commands->postbuild = array();

  // Get config file.
  if (!$config_file) {
    $config_file = buildmanager_get_config_file();
  }
  drush_log(dt('Loading configuration from: !config_file', array('!config_file' => $config_file)), 'ok');
  $config = buildmanager_get_config($config_file);

  // Assemble $info array by loading, parsing and merging make file(s).
  $build_file = $config['build_file'];
  drush_log(dt('Using build file: !build_file', array('!build_file' => $build_file)), 'ok');
  $info = _buildmanager_get_info($build_file);

  // If requested, output full build info.
  if (drush_get_option('show-info')) {
    drush_print('$info:');
    drush_print_r($info);
  }

  // Add buildmanager's prebuild commands.
  $target = $config['target'];
  if (file_exists($target)) {
    // Remove Drupal code base. To be rebuilt by drush make.
    $commands->prebuild[] = "rm -rf $target";
  }

  // Add custom prebuild commands from config file.
  foreach ($config['prebuild-commands'] as $command) {
    $commands->prebuild[] = $command;
  }

  // Add buildmanager's postbuild commands.
  if (!drush_get_option('no-commit', FALSE)) {
    // Commit.
    $message = ($message = drush_get_option('message', FALSE)) ? $message : dt('Rebuild with drush buildmanager.');
    $commands->postbuild[] = _buildmanager_commit($message);
  }

  // Add custom postbuild commands from config file.
  foreach ($config['postbuild-commands'] as $command) {
    $commands->postbuild[] = $command;
  }

  // Invoke buildmanager_build. Give other modules/extensions an opportunity to
  // examine $info and $config and add or modify $commands.
  drush_command_invoke_all('buildmanager_build', $info, $config, $commands);

  // Execute prebuild commands.
  foreach ($commands->prebuild as $command) {
    // @todo Add error handling.
    drush_shell_exec($command);
  }

  // Build. (Run drush make.)
  // If target director already exists, remove it.
  // TODO Get build config. Use that to build this command.
  $build_command = DRUSH_COMMAND . " make {$build_file} {$target} --no-recursion --no-gitinfofile";
  drush_shell_exec($build_command);

  // Execute postbuild  commands.
  foreach ($commands->postbuild as $command) {
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
function buildmanager_get_config($config_file) {
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

/**
 * Get info for a particular project from make file(s).
 *
 * @param string $subtree_name.
 *   Name of project/subtree.
 *
 * @param array $config
 *   Config from buildmanager.config.yml
 *
 * @return array
 *   Info from make file(s).
 */
function buildmanager_get_project_info($subtree_name, $config) {
  $build_file = $config['build_file'];
  $info = _buildmanager_get_info($build_file);
  if (!isset($info['projects'][$subtree_name])) {
    drush_set_error('buildmanager', dt('Sorry. No info available in make file(s) for project: !name',
      array('!name' => $subtree_name)));
  }
  return $info['projects'][$subtree_name];
}
