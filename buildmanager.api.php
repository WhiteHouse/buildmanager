<?php

/**
 * Implements hook_buildmanager_build().
 *
 * Run any setup or validation that needs to be done at runtime during
 * buildmanager-build. Abort build if necessary. Otherwise Add or update
 * prebuild or postbuild commands by making changes to the $commands object.
 *
 * If your extension supports additional Build Manager configuration (e.g. Drush
 * Subtree enables users to store info about subtrees in
 * buildmanager.config.yml), access that config info and act on it here.
 *
 * @param array $info
 *   Info from drush make build file and any included make files.
 *
 * @param array $config
 *   Parsed buildmanager.config.yml file, passed in here as one big array.
 *
 * @param object $commands
 *   Commands to be executed by buildmanager. Update this object to
 *   add/update/remove commands.
 *   - precommands (array), commands to execute before `drush make` (re)build
 *   - postcommands (array), commands to execute after `drush make` (re)build
 *
 * @return bool|int|string|array
 *   Returning anything will abort build.
 */
function example_buildmanager_build($info, $config, $commands) {
  // Check for examples in $config.
  if (!isset($config['examples'])) {
    // This project has no examples. Our work here is done.
    return;
  }

  // Loop through all examples described in buildmanager config to add prebuild
  // and postbuild commands.
  foreach ($config['examples'] as $example_name => $properties) {
    // Do something with examples here.
    $prebuild_command = "echo 'This is a prebuild command: {$example_name}'";
    $postbuild_command = "echo 'This is a postbuild command: {$example_name}'";

    // Add prebuild commands.
    $commands->prebuild[] = $prebuild_command;

    // Add postbuild commands.
    $commands->postbuild[] = $postbuild_command;
  }

  // If something went wrong, abort build.
  $fail = example_check_for_failure();
  if ($fail) {
    // User canceled build.
    drush_log(dt('Example: Canceling build.'), 'error');
    // Returning anything cancels the build.
    return 'Example: Canceling build.';
  }

  // No return necessary. Commands added to $command object are available to
  // buildmaster_build command now.
}

/**
 * Implements hook_buildmanager_build_options().
 *
 * @return array
 *   Array of drush options to be added to the buildmanager-build command. (This
 *   gets invoked and included in the command when Build Manager invokes
 *   hook_drush_command.)
 */
function example_buildmanager_build_options() {
  return array(
    'example-info' => array(
      'description' => 'Print info about examples to screen during build.',
    ),
  );
}

/**
 * Implements hook_buildmanager_configure().
 *
 * Hook into buildmanager-configure's interactive prompt. Prompt user for
 * relevant info, then incorporate that into the $config array which will be
 * generated for user and stored in buildmanager.config.yml
 *
 * param array $config
 *   See hook_buildmanager_configure.
 *
 * pararm bool $prompt
 *   Ask user if project includes subtrees.
 *
 * @return array
 *   Config to be written to buildmanager.config.yml with any additional
 *   properties added.
 */
function example_buildmanager_configure($config, $prompt = TRUE) {
  if ($prompt && drush_confirm("\n" . dt("Does your build include examples?")) == FALSE) {
    // Our work here is done. Return unmodified $config array.
    return $config;
  }
  else {
    drush_print("\n" . dt("I'll help you set up configuration for examples."));
  }

  // Get example properties.
  $get_examples = TRUE;
  $examples = array();
  while ($get_examples) {
    $name = drush_prompt(dt("What's the name of your example?"));
    $something1 = drush_prompt(dt("Enter something."));
    $something2 = drush_prompt(dt("Enter something else."));

    $examples[$name]['something1'] = $something1;
    $examples[$name]['something2'] = $something2;

    // Continue collecting info about more examples?
    $get_examples = drush_confirm(dt("Does your build include any more examples?"));
  }

  // Show examples to user. Confirm they're right.
  drush_print("\n" . dt("Here are the examples you entered:" . "\n"));
  drush_print(drush_format($examples, NULL, 'yaml'));
  if (!drush_confirm("Are these properties correct?")) {
    // Do over.
    $examples = examples_buildmanager_configure($config);
  }

  // Set examples property in Build Manager config file.
  $config['examples'] = $examples;

  // Return updated $config for buildmanager.config.yml.
  return $config;
}

/**
 * Implements hook_buildmanager_parse_error_output().
 *
 * When a prebuild or postbuild command fails, Build Manager invokes this hook
 * to give extensions an opportunity to examine the error and then take action
 * or surface helpful additional information for end user.
 *
 * @param array $output
 *   @see drush_shell_exec_output()
 */
function example_buildmanager_parse_error_output($output) {
  // Do something with $output. Then generate a helpful error message for end
  // user.
  $message = example_parse_error_output($output, FALSE);
  if ($message) {
    // If a message was returned, log it (this will print to screen and/or log).
    drush_log($message, 'error');
  }
}
