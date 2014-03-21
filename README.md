Build Manager
==============


Overview
---------

Build Manager provides a wrapper around Drush's make command to simplify
builds for people who maintain distros or custom sites with drush make. Here's
how it simplifies things:

  - Provides an interactive prompt for setting up your Build Manager
    configuration. Once it's set up, Build Manager can automate your entire
    build.

  - Enables site maintainers to specify custom prebuild and postbuild commands to be
    run before and after `drush make`.

  - Enables other drush extensions (like Drush Subtree) to hook in and provide
    their own prebuild and postbuild commands and support for additional
    configuration options.


Dependencies
------------

  - (Recommended) Drush master / 7.x (Build Manager uses the Drush Make
    --no-recurse flag and autoloading which--as of the time of this
    writing--have not been backported to 6.x)


Usage
-----

If you already have a build.make file, get started quickly by using the interactive configure prompt:

    # Start interactive configure prompt. This will generate a YAML config file
    # for you. (By default, it's called buildmanager.config.yml.)
    drush buildmanager-configure

    # Now (re)build your code base like this.
    drush buildmanager-build

If you do not have make file set up yet, or if you're using [Drush
Subtree](https://github.com/whitehouse/drushsubtree) to incorporate git subtrees
into your site's make build, see the section below [Tips for working with make
files]()

[[ Outline ]]

 Config includes:
 - prebuild-commands
 - postbuild-commands
 - build properties

[[ Developers ]]

 Other extensions can implement the following hooks in their my-project.drush.inc
 files:
   - hook_buildmanager_build($make_info, $build_config, $commands), obj $commands
   - hook_buildmanager_build_options(), returns addtional options to include in
     buildmanager-build
   - hook_buildmanager_configure($config), returns altered $config
 
 Implementers can support arbitrary config in $build_config and adjust commands
 accordingly.


  1. Set up a build.make file at the top-level of your repository. Drush make
     will use this to set up your site. (NOTE: You can include other make files in you
     your build.make, and included make files can include other make files. But
     when drush make runs with the --no-recursion flag. If your make file
     downloads a project with its own make file, drush make will NOT
     automatically build the stuff specified by that make file.)

     If your build.make includes a makefile from a git subtree, add it to your
     repo like this:

       git subtree add --prefix=projects/example --squash --message="Added tweetserver subtree. From https://github.com/example/example.git" https://github.com/example/example.git 7.x-1.x
       

  2. Set up config for your own site build in drushsubtree.example.yml. This
     includes (see drushsubtree.example.yml): 
     
       - which build file to use (e.g. build.make)
       - where to build the site (e.g. docroot)
       - what projects to replace with git subtrees
       - any commands you want to run after the site is rebuilt


  3. Do this:
      
        cd /path/to/my-site-repo
        drush drushsubtree-build --message="Update example distro to 7.x-1.3 with drush subtree" -v
          
          or use aliases

        drush subtree-build --message="Update example distro to 7.x-1.3 with drush subtree" -v
        drush dsb --message="Update example distro to 7.x-1.3 with drush subtree" -v

        # If you have multiple config files, you can skip the prompt and specify
        # which config to use like this:
        drush subtree ./drushsubtree.mysite.yml --message="Update example distro to 7.x-1.3 with drush subtree" -v

     Helpful additional options provided by Drush:

        # Use --debug to see more info about what drush is doing under the hood.
        drush dsb -v --message="Update example distro to 7.x-1.3 with drush subtree"
        drush dsb -v --debug --message="Update example distro to 7.x-1.3 with drush subtree"
 
        # Use --simulate to see the commands drush will execute when you run
        # site make (without actually running it).
        drush dsb --message="Update example distro to 7.x-1.3 with drush subtree" --simulate


Tips for working with make files
--------------------------------

If you're including multiple make files, all properties need keys. For example...

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

TODO
-----
Currently subtree turns off recursion and expects all make files to be
included in a master build.make. It could be possible to get rid of this, but
this could create confusion and complexity. Revisit this.

