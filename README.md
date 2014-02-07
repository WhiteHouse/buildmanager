Gotchas
-------

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

