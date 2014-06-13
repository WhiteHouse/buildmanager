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
      'config_file' => '(Optional) path/to/buildmanager.config.yml',
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

  $items['buildmanager-configure'] = array(
    'description' => "Interactive prompt to set up your buildmanager.config.yml.",
    'arguments' => array(
      //'config' => '(Optional) path/to/buildmanager.config.yml to be updated',
    ),
    'examples' => array(
      'drush buildmanager-configure' => '',
      'drush buildmanager-config' => '',
      'drush bmc' => '',
    ),
    'aliases' => array('bmc', 'buildmanager-config'),
    // No bootstrap at all.
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );

  $items['buildmanager-find-make-files'] = array(
    'description' => "Find any make files in the repo.",
    'arguments' => array(
      //'config' => '(Optional) path/to/buildmanager.config.yml to be updated',
    ),
    'examples' => array(),
    'aliases' => array('bmfmf'),
    // No bootstrap at all.
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );

  return $items;
}

/**
 * Generate buildmanager.config.yml.
 */
function drush_buildmanager_configure() {
  drush_print("\n" . dt("Beginning interactive prompt to set up your buildmanager config file..."));
  drush_print(dt("Hit <Enter> to accept defaults.") . "\n");

  // Get config file name.
  drush_print(dt("Enter the file name where your YAML configuration will be stored."));
  $prompt = dt('file name');
  $default = 'buildmanager.config.yml';
  $required = TRUE;
  $file_name = drush_prompt($prompt, $default, $required);
  // @todo Check name. Prompt: Note: buildmanager only auto-detects config files that begin with "buildmanager. Change it?

  // Construct $config array to be converted to YAML and written to $file_name at the end.
  $config = array();

  // Get build properties.
  drush_print("\n" . dt("build properties..."));

  // Target.
  drush_print(dt("Enter the name of the directory where codebase will be built."));
  $prompt = dt('target');
  $default = 'docroot';
  $required = TRUE;
  $config['build']['target'] = drush_prompt($prompt, $default, $required);

  // Build file.
  drush_print(dt("Enter the name of the drush make file to be used to build your Drupal codebase.\n(If your make file is not stored in the top directory of your repo, specify /path/to/you/build.make.)"));
  $prompt = dt('build_file');
  $default = 'build.make';
  $required = TRUE;
  $config['build']['build_file'] = drush_prompt($prompt, $default, $required);

  // Options.
  drush_print(dt("Enter comma separated options to be passed to drush make."));
  $prompt = dt('options');
  $default = '';
  $required = FALSE;
  if ($options = drush_prompt($prompt, $default, $required)) {
    $config['build']['options'] = explode(',', $options);
  }

  // Enable other Drush extensions to hook in and add config.
  $config = drush_command_invoke_all('buildmanager_configure', $config);

  // Add custom postbuild commands to restore symlinks.
  if (drush_confirm("\n" . dt("Does your codebase contain symlinks?\n(e.g to a sites directory or settings.php file that lives outside your docroot)"))) {
    $symlink_commands = _buildmanager_bmc_get_symlink_commands();
    // Check for any other postbuild commands set by anyone implementing hook_buildmanager_configure.
    $other_postbuild_commands = (isset($config['postbuild-commands'])) ? $config['postbuild-commands'] : array();
    // Add symlink commands to existing postbuild commands.
    $config['postbuild-commands'] = array_merge($other_postbuild_commands, $symlink_commands);
  }

  // Format config as YAML.
  $config_yaml = drush_format($config, NULL, 'yaml');

  // Print config to screen.
  drush_print("\n" . dt("Here's the configuration you entered formatted as YAML:") . "\n");
  drush_print($config_yaml);

  // Write to file.
  file_put_contents($file_name, $config_yaml);
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

  // Get HEAD commit, so we can help users get untangled if the build fails.
  exec('git rev-parse HEAD', $output, $exit_code);
  $head = $output[0];

  // Get config file.
  if (!$config_file) {
    $config_file = buildmanager_get_config_file();
  }
  drush_log(dt('Loading configuration from: !config_file', array('!config_file' => $config_file)), 'ok');
  $config = buildmanager_get_config($config_file);

  // Assemble $info array by loading, parsing and merging make file(s).
  $build_file = $config['build']['build_file'];
  drush_log(dt('Using build file: !build_file', array('!build_file' => $build_file)), 'ok');
  $info = _buildmanager_get_info($build_file);

  // If requested, output full build info.
  if (drush_get_option('show-info')) {
    drush_print('$info:');
    drush_print_r($info);
  }

  // Add buildmanager's prebuild commands.
  $target = $config['build']['target'];
  if (file_exists($target)) {
    // Remove Drupal code base. To be rebuilt by drush make.
    $commands->prebuild[] = "rm -rf $target";
  }

  // Add custom prebuild commands from config file.
  if (isset($config['prebuild-commands'])) {
    foreach ($config['prebuild-commands'] as $command) {
      $commands->prebuild[] = $command;
    }
  }

  // Add custom postbuild commands from config file.
  if (isset($config['postbuild-commands'])) {
    foreach ($config['postbuild-commands'] as $command) {
      $commands->postbuild[] = $command;
    }
  }

  // Add buildmanager's postbuild commands. Add after custom commands, so our
  // commit includes all changes.
  if (!drush_get_option('no-commit', FALSE)) {
    // Commit.
    $message = ($message = drush_get_option('message', FALSE)) ? $message : dt('Rebuild with drush buildmanager.');
    $commands->postbuild[] = _buildmanager_commit($message);
  }

  // Invoke buildmanager_build. Give other modules/extensions an opportunity to
  // examine $info and $config, add or modify $commands, or abort.
  $abort = drush_command_invoke_all('buildmanager_build', $info, $config, $commands);
  if ($abort) {
    return drush_set_error(dt('Build Manager: Aborting build.'));
  }

  // Build. (Run drush make.)
  $options = isset($options) ? implode(' ', $config['build']['options']) : '';
  $build_command = DRUSH_COMMAND . " make {$build_file} {$target} {$options}";

  // Reorganize as one big array of commands. Then we'll run them in order:
  // Prebuild, build, postbuild.
  $commands = array_merge(
    $commands->prebuild,
    array($build_command),
    $commands->postbuild
  );

  // Execute.
  foreach ($commands as $command) {
    $success = drush_shell_exec($command);
    if (!$success) {

      // Command failed. Notify user and with suggested next steps.
      drush_log(dt('Command failed: !command', array('!command' => $command)), 'error');
      $output = drush_shell_exec_output();
      drush_log(implode("\n", $output), 'error');
      $reset_command = "git reset --hard {$head}";
      $error_message = dt('Review error message from shell command above to see why this build did not complete successfully. To reset repo to the state it was in before begnning this build you can do this: !fix',
        array('!fix' => "\n\t{$reset_command}"));
      drush_log($error_message, 'error');

      // Give other Drush extensions an opportunity to handle error message.
      drush_command_invoke_all('buildmanager_parse_error_output', $output);

      // Offer to reset repo for user.
      if (drush_confirm(dt("Do a hard reset to undo Build Manager's changes?"))) {
        drush_shell_exec($reset_command);
        $output = drush_shell_exec_output();
        drush_log(implode("\n", $output), 'notice');
      }

      // Exit.
      return;
    }
  }

}

/**
 * Implements drush_hook_COMMAND_validate().
 */
function drush_buildmanager_build_validate() {
  $is_valid = TRUE;

  // TODO This command must be run from top-level of git repo.
  $is_valid = buildmanager_is_toplevel();

  // TODO Check drush version. Currently, this requires master branch with
  // --no-recursion option.
  // TODO Check ownership. If ant rebuilds, ownership may be funny.
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
 * Check if $path is the toplevel of the repository.
 *
 * @param string $path
 *   Optional path. Use directory command was executed from otherwise.
 *
 * @return bool
 *   True, success. False, fail.
 */
function buildmanager_is_toplevel($path = '') {
  // Get path to check.
  if (!$path) {
    exec('pwd', $output, $exit_code);
    $path = $output[0];
  }

  // Get toplevel of repo.
  $toplevel = buildmanager_get_toplevel();

  // Make sure we're working with real paths.
  $path = realpath($path);
  $toplevel = realpath($toplevel);

  // Compare.
  if ($path == $toplevel) {
    return TRUE;  
  }
  else {
    return drush_set_error(dt('This command must be run from the toplevel directory in the repository. Change directories, then try again: !cd',
      array('!cd' => "\n\tcd {$toplevel}")));
  }
}

/**
 * @return string|bool
 *   Path to toplevel of repo.
 */
function buildmanager_get_toplevel() {
  $command = 'git rev-parse --show-toplevel';
  exec($command, $output, $exit_code);
  if ($exit_code === 0) {
    // Success.
    return $output[0];
  }
  else {
    // Could not get toplevel of repo. Maybe we're not in a repo.
    return drush_set_error(dt('Command failed: git rev-parse --show-toplevel. Are you in a git repo?'));
  }
}

/**
 * Callback for buildmanager-find-make-files.
 */
function drush_buildmanager_find_make_files() {
  // Scan directory for make files like *.make.
  foreach (buildmanager_find_make_files() as $file) {
    drush_print("{$file['basename']}\t\t{$file['filename']}");
  }
}

/**
 * Find all makefiles in repo.
 */
function buildmanager_find_make_files() {
  $toplevel = buildmanager_get_toplevel();
  $make_files = array();

  // Scan directory for make files like *.make.
  foreach (drush_scan_directory($toplevel, '/.*/') as $file) {
    if (!strpos($file->basename, '.make')) {
      continue;
    }
    $make_files[] = array(
      'basename' => $file->basename,
      'filename' => $file->filename,
    );
  }

  return $make_files; 
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
    return drush_set_error(dt('No config file found. See buildmanager/README.md.'));
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
      if (isset($array2[$key]) && is_array($array2[$key])) {
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
  $command = "git add --all; git commit -am '{$message}' --no-verify;";
  return $command;
}

/**
 * Get info for a particular project from make file(s).
 *
 * @param string $name
 *   Name of project/subtree.
 *
 * @param array $config
 *   Config from buildmanager.config.yml
 *
 * @return array
 *   Info from make file(s).
 */
function buildmanager_get_project_info($name, $config) {
  $build_file = $config['build']['build_file'];
  $info = _buildmanager_get_info($build_file);
  if (!isset($info['projects'][$name])) {
    drush_set_error('buildmanager', dt('Sorry. No info available in make file(s) for project: !name',
      array('!name' => $name)));
  }
  return $info['projects'][$name];
}

/**
 * Get Drupal core version.
 *
 * @param array $config
 *   Config from buildmanager.config.yml
 *
 * @return array
 *   Info from make file(s).
 */
function buildmanager_get_core_version($config) {
  $build_file = $config['build']['build_file'];
  $info = _buildmanager_get_info($build_file);
  if (!isset($info['core'])) {
    drush_set_error('buildmanager', dt('Sorry. No core version specified: !here',
      array('!here' => $build_file)));
  }
  return $info['core'];
}

/**
 * Prompt user to for symlinks. Generate postbuild symlink commands.
 *
 * @return array
 *   Postbuild shell commands to restore symlinks after drush make blows them away.
 */
function _buildmanager_bmc_get_symlink_commands() {
  drush_print("Collecting info for postbuild commands to restore symlinks...");
  $commands = array();
  $symlinks = array();

  // Get info about symlinks.
  $get_links = TRUE;
  while ($get_links) {
    // @todo Add some validation. E.g. don't accept spaces.
    $path_to_symlink = drush_prompt(dt("Path to symlink")); 
    $path_to_target = drush_prompt(dt("Path to file or directory"));
    $symlinks = array_merge($symlinks, array($path_to_symlink => $path_to_target));
    // Review symlinks entered.
    drush_print("\n" . dt("These are the symlinks you have entered:"));
    foreach ($symlinks as $from => $to) {
      drush_print("{$from} -> {$to}");
    }
    drush_print("\n");
    $get_links = drush_confirm(dt("Enter more symlinks?"));
  }

  // Generate symlink commands.
  foreach ($symlinks as $from => $to) {

    // Get source, relative to parent directory inside docroot.
    $count = count(explode('/', $from));
    $source = '';
    for ($i = 0; $i < $count - 1; $i++) {
      $source .= '../';
    };
    // Add prefix to relative source path.
    $source .= $to;

    // Parent directory for symlink.
    $pos = strrpos($from, '/');
    $parent_directory = substr($from, 0, $pos);

    // Name of symlink.
    $name = substr($from, $pos + 1);

    // Remove the directory. Replace it with a symlink.
    $commands[] = "rm -rf {$from}; cd {$parent_directory}; ln -s {$source} {$name};";
  }

  // Show commands to user. Confirm they're right.
  drush_print("\n" . dt("Here are the postbuild commands Build Manager will run from the top-level of your repository after Drush Make blows away your symlinks:"));
  drush_print(drush_format($commands,  NULL, 'yaml'));
  if (!drush_confirm("Are these postbuild commands correct?")) {
    // Do over.
    $commands = _buildmanager_bmc_get_symlink_commands();
  }

  return $commands;
}
