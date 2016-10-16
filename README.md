Build Manager
==============

Contents
--------

- [Overview](#overview)
- [Dependencies](#dependencies)
- [Usage](#usage)
  - [Quick start](#quick-start)
  - [Manual setup](#manual-setup)
  - [(Re)Build](#rebuild)
  - [Developers](#developers)
- [Tips for working with make files](#tips-for-working-with-make-files)
  - [Example 1: Managing my custom site with a build.make](#example-1-managing-my-custom-site-with-a-buildmake)
  - [Example 2: Managing my copy of someone else's distro with build.make](#example-2-managing-my-copy-of-someone-elses-distro-with-buildmake)
  - [Example 3: Managing my instance of a distro I maintain with build.make](#example-3-managing-my-instance-of-a-distro-i-maintain-with-buildmake)
    - [Including multiple files in build.make (all properties need keys)](#including-multiple-files-in-build-make-all-properties-need-keys)

Overview
---------

Build Manager provides a wrapper around Drush's Make command to simplify
builds for people who maintain distros or custom sites with Drush Make. Here's
how it simplifies things:

  - Provides an interactive prompt for setting up your Build Manager
    configuration. Once it's set up, Build Manager can automate your entire
    Drush Make build.

  - Enables site maintainers to specify custom prebuild and postbuild commands to be
    run before and after `drush make`.

  - Enables other drush extensions (like Drush Subtree) to hook in and provide
    their own prebuild and postbuild commands and support for additional
    configuration options.


Dependencies
------------

  - Drush master / 7.x (Note: There's not actually a good reason we have this
    dependency on 7.x anymore. The Drush Make --no-recursion flag in Drush 7.x is
    optional.)
    @todo Make Build Manager backwards compatible with Drush 6.x


Usage
-----

### Quick start

If you already have a build.make file, get started quickly by using the interactive configure prompt:

    # Start interactive configure prompt. This will generate a YAML config file
    # for you. (By default, it's called buildmanager.config.yml.)
    drush buildmanager-configure

    # Now (re)build your code base like this.
    drush buildmanager-build

To see what's going on under the hood, use Drush's `-v` option. To see all the
commands Build Manager would execute to rebuild your repo without actually
running any of them, use Drush's `--simulate` option.

If you do not have make file set up yet, or if you're using [Drush
Subtree](https://github.com/whitehouse/drushsubtree) to incorporate git subtrees
into your site's make build, see the section below [Tips for working with make
files](#tips-for-working-with-make-files).


### Manual setup

  1. Set up a build.make file at the top-level of your repository. Drush Make
     will use this to (re)build your code base.

  1. Set up config for your own site build in buildmanager.example.yml,
     following one of the example config files included with Build Manager.


### (Re)Build

Do this:
      
        cd /path/to/my-site-repo
        drush buildmanager-build --message="Build Manager: Update example distro to 7.x-1.3."
          
        # If you have multiple config files, you can skip the prompt and specify
        # which config to use like this:
        drush bmb drushsubtree.mysite.yml --message="Build Manager: Update example distro to 7.x-1.3" -v


### Developers

 Implement the following hooks to extend Build Manger with your own Drush
 projects (also see documentation in buildmanager.api.php and see working
 examples for all of these in [Drush Subtree](https://github.com/whitehouse/drushsubtree)):

   - hook_buildmanager_build, add/update prebuild and postbuild commands or
     abort build
   - hook_buildmanager_build_options, return additional options to include in
     buildmanager-build 
   - hook_buildmanager_configure, insert your extension into the
     `buildmanager-configure` interactive prompt to generate additional config
     to be stored in buildmanager.config.yml
   - hook_buildmanager_parse_error_output, when commands fail, do something


Tips for working with make files
--------------------------------

Build Manager is a simple wrapper around Drush Make for working with build
files. A "build" file is drush make file that kicks off a Drush Make build for a
Drupal site codebase. (Drupal distros on drupal.org usually include several make files.
The build file is the one named: build-myprofile.make.)

You can use Build Manager with any build file, and configure it to use and Drush
Make files you want BUT here are a few recommendations to keep things easy and
straight forward working with Build Manager:

  - Your toplevel build file (and any make files it includes) should be stored outside
    your Drupal code base.
  - If you're build file has multiple includes, all included make files need keys. (See
    example below.)

### Example 1: Managing my custom site with a build.make

        core = 7.x
        api = 2

        ; Core version
        projects[drupal][version] = 7.25

        ; Contrib projects and versions
        projects[ctools][version] = 1.3
        projects[views][version] = 3.1


### Example 2: Managing my copy of someone else's distro with build.make

        core = 7.x
        api = 2

        ; Core
        ; ----
        projects[drupal][version] = 7.25

        ; Install profile (distro)
        ; ------------------------
        projects[dkan][type] = profile
        projects[dkan][tag] = 7.x-1.0
        projects[dkan][download][type] = git 

        ; Overrides
        ;
        ; Override included make files and projects included by DKAN. Contrib projects
        ; here are stored in sites/all/modules and sites/all/themes buy default.
        ; ----------------------------------------------------------------------------

        ; Field Group
        ;
        ; "Show" appears in horizontal tab title if node form submitted through ajax
        ; See https://drupal.org/node/2042681
        ;
        projects[field_group][subdir] = dkan
        projects[field_group][version] = 1.3
        projects[field_group][patch][2042681] = https://drupal.org/files/issues/field-group-show-ajax-2042681-8.patch

        ; Reference Field Synchronization
        ;
        ; Performance Killer: Entity Load is loading 1 node at a time, instead of multiple. with patch.
        ; See https://drupal.org/node/1928680
        ;
        ; Errors when deleting content that contains an entity reference field
        ; See https://drupal.org/node/1864670
        ;
        projects[ref_field][type] = module
        projects[ref_field][download][type] = git
        projects[ref_field][download][url] = http://git.drupal.org/project/ref_field.git
        projects[ref_field][download][revision] = 9dbf7cfa17172966a3b486e5ae2486f21faff66b
        projects[ref_field][subdir] = dkan
        projects[ref_field][patch][1928680] = https://drupal.org/files/ref_field_sync-entity-load-multiple-1928680-1.patch
        projects[ref_field][patch][1864670] = https://drupal.org/files/issues/ref_field-delete_bug-1864670-2.patch

        ; Contrib
        ;
        ; Include site-specific contrib projects here.
        ; ----------------------------------------------

        projects[devel][version] = 1.3
        projects[devel][subdir] = contrib

        projects[features_override][version] = 2.0-rc1
        projects[features_override][subdir] = contrib

        projects[features_extra][version] = 1.0-beta1
        projects[features_extra][subdir] = contrib


### Example 3: Managing my instance of a distro I maintain with build.make

Recommended: Do not nest contrib projects inside (e.g. modules or themes) inside
an install profile you maintain. It makes git workflows a lot more confusing and
complicated. A better option is to (1) keep a copy the install profile outside
docroot, then symlink it into docroot/profiles/example, and (2) to use the
Drush 7's --no-recursion flag so Drush Make doesn't detect included make files
and next projects inside projects, instead include all the make files you want
to use in your build.make file.

        includes[base] = projects/example/build-example.make
        includes[contrib] = projects/example/drupal-org.make

        ; Instance-specific overrides of makefiles above go below here.

        ; Set default subdirectory for modules included by distro.
        defaults[projects][subdir] = example

        ; Do not apply default above to example profile.
        projects[example][subdir] = ''


#### Including multiple files in build.make (all properties need keys)

This is bad:

    example1.make has this line: includes[] = exampleA.make
    example2.make has this line: includes[] = exampleB.make

This is good:

    example1.make has this line: includes[a] = exampleA.make
    example2.make has this line: includes[b] = exampleB.make

And this is good:

    build-example.make has this line:  includes[base] = path/to/base.make
    base.make has this line:           includes[core] = drupal-org-core.make
    base.make also has this line:      includes[contrib] = drupal-org.make
