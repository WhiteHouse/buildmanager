<?php
/**
 * @file
 */
require_once('drushsubtree.commander.class.inc');

/**
 * SubtreeCommander class.
 */
class SubtreeCommander extends Commander {
  private $name;
  private $uri;
  private $branch;
  private $message;
  private $squash;
  private $pull;
  private $merge;
  /**
   * Construct SubtreeCommander object. Fill in properties from config and defaults.
   *
   * @param string name
   *   Key for subtree in buildmanager.config.yml.
   *
   * @param array properties
   *   Properties for subtree provided in buildmanager.config.yml.
   */
  public function __construct($name, $properties) {
    $this->name = $name;
    $this->uri     = (isset($properties['uri']))    ? $properties['uri']    : '';
    $this->branch  = (isset($properties['branch'])) ? $properties['branch'] : '';
    $this->message = (isset($properties['message']))? $properties['message']: '';
    $this->squash  = (isset($properties['squash'])) ? $properties['squash'] : TRUE;
    $this->pull    = (isset($properties['pull']))   ? $properties['pull']   : TRUE;
    $this->merge   = (isset($properties['merge']))  ? $properties['merge']  : TRUE;
  }

  /**
   * Generate git subtree add command to be run from top-level of repo.
   *
   * This command is stored in SubtreeCommander's command property.
   *
   * @return string|bool
   *   Executable shell command. False for don't do anything.
   */
  function add() {
    $prefix = _drushsubtree_get_prefix($this->name);

    if (file_exists($prefix)) {
      // Subtree has already been created. Or there's a directory in our way.
      // Skip this step.
      drush_log(dt("This directory already exists: !prefix", $prefix), array('!prefix' => $prefix), 'notice');
      drush_log(dt("Git subtree add command will be skipped. If !prefix is not a subtree, remove this directory, commit, then re-try.", $prefix), array('!prefix' => $prefix), 'warning');
      // We're done. Return.
      return FALSE;
    }

    // Make sure we have minimum required $details.
    $uri = $this->uri;
    $branch = $this->branch;
    if (!$this->uri || !$this->branch) {
      // We're done. Return.
      return FALSE;
    }

    // Squash commit history of subtree into a single commit?
    $squash = ($this->squash) ? ' --squash' : '';

    // Add custom commit message?
    $message = _drushsubtree_add_subtree_commit_message($this->message);

    // Build the command to add subtree for the first time.
    $command = "git subtree add --prefix={$prefix} {$squash} {$message} $uri $branch";
    // Set commander command.
    $this->command = $command;

    return $command;
  }

  /**
   * Generate git subtree pull command to be run from top-level of repo.
   *
   * This command is stored in SubtreeCommander's command property.
   *
   * @return string|bool
   *   Executable shell command. False for don't do anything.
   */
  function pull() {
    $directory = $this->name;

    // Check to see if pulls are disabled (e.g. for local development).
    if ($this->pull == FALSE) {
      drush_log(dt('git subtree pull is disabled for !project', array('!project' => $directory)), 'warning');
      return '';
    }

    drush_log(dt('Preparing to pull subtree: !subtree_name', array('!subtree_name' => $directory)), 'ok');

    // Get params for git subtree pull command.
    $prefix = _drushsubtree_get_prefix($directory);
    $uri = $this->uri;
    $branch = $this->branch;

    // Add custom commit message?
    $message = _drushsubtree_add_subtree_commit_message($this->message);

    // Squash commit history of subtree into a single commit?
    $squash = ($this->squash) ? ' --squash' : '';

    $this->command = "git subtree pull --prefix={$prefix} {$squash} {$message} {$uri} {$branch}";

    return $this->command;
  }

  function checkout() {}

  function push() {}

  function merge() {}

  /**
   * Getters and setters for properties.
   */
  function getName() {
    return $this->name;
  }
  function setName($name) {
    $this->name = $name;
  }

  function getUri() {
    return $this->uri;
  }
  function setUri($uri) {
    $this->uri = $uri;
  }

  function getBranch() {
    return $this->branch;
  }
  function setBranch($branch) {
    $this->branch = $branch;
  }

  function getMessage() {
    return $this->message;
  }
  function setMessage($message) {
    $this->message = $message;
  }

  function getSquash() {
    return $this->squash;
  }
  function setSquash($squash) {
    $this->squash = $squash;
  }

  function getPull() {
    return $this->pull;
  }
  function setPull($pull) {
    $this->pull = $pull;
  }

  function getMerge() {
    return $this->merge;
  }
  function setMerge($merge) {
    $this->merge = $merge;
  }

}
