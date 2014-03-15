; Base
;
;  Builds an instance of the example_profile distro.
;  This example assumes you maintain an distribution (install profile) on
;  drupal.org called example_profile. This include simply pulls in the make file
;  included in that project (stored in this repo at projects/example/build-example.make).
;
; --------------------------------------------------
includes[base] = projects/example/build-example.make

; Contrib
;
;  Add contrib projects to your own copy of the example distro.
;
; ------------------------------------------------------------
projects[ctools][version] = 1.3
projects[context][version] = 3.1
projects[entity][version] = 1.2
projects[entityreference][version] = 1.1
