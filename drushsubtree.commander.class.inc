<?php
/**
 * @file
 * Provides abstract class for drush subtree commander classes.
 */

/**
 * Abstract Commander class.
 */
abstract class Commander {
  private $command;

  /**
   * Get command property.
   *
   * @return string
   *   Command to be executed.
   */
  public function getCommand() {
    return $this->command;
  }

  /**
   * Set command property.
   *
   * @param string $command
   *   Command to be executed.
   */
  public function setCommand($command) {
    $this->command = $command;
  }

  /**
   * Add a shell command after the command stored in $this->command.
   *
   * @param string $command
   *   Command to be executed in addition to whatever is already stored in
   *   $command property.
   */
  public function appendCommand($command) {
    $this->command .= $command;
  }

  /**
   * Execute command.
   *
   * @return array
   *   Comand and exec results.
   *   - command (string)
   *   - output (array)
   *   - exit_code (int)
   */
  public function exec() {
    exec($this->command, $output, $exit_code);
    return array(
      'command' => $this->command,
      'output' => $output,
      'exit_code' => $exit_code,
    );
  }

}
