Drush Subtree
===============

About
-----

  Drush Subtree provides a simple wrapper around Drush's make command. The purpose is to:
   
    1. Make it easy to maintain your own instance of a Drupal distro.
    2. Reduce friction for contributors.

  Drush Subtree makes it easier to maintain an instance of a distro by:

    - Putting all your contrib and custom projects into a build.make file. This
      way any patches you maintain can be easily documented and applied by drush
      make. 

    - Storing any custom shell commands in a simple config file (see
      drushsubtree.example.yml) to automate and standardize your site re-build
      process. This makes updating to the next release as simple as running
      `drush make`.

  Drush Subtree reduces friction for contributors by storing contrib and custom
  projects you (or your team) maintain in git subtrees. This enables you to:

    - Do development inside whatever site repo you're actively working in,
      then push commits out of your site repo up to drupal.org or whatever
      private repo your custom module may live in.

    - Pull updates into your site repo from an outside repo for your contrib
      distro, module, theme, or custom project. 

    - (For large teams) Enables people to contribute to your contrib project
      with internal work on a site repo, then lets you easily push that work out
      to public repos when it's ready to be released.

Dependencies
------------

  - Drush master branch (until --no-recurse and autoloading are committed to 6.x)

  - Git subtree

Usage
-----

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


Tips for including multiple make files in your build file
----------------------------------------------------------

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

