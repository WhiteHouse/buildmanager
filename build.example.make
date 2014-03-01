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

; Subtrees
;
;  For contrib projects you maintain, where it's convenient to do development
;  inside your site repo, replace drush downloads with git subtrees. 
; 
;  Add commit IDs corresponding with tagged project releases
;  where we will replace drush downloads with local git subtrees.
; 
;  If you're not sure what the commit ID you need is, look it up like this: 
; 
;    git show <tag I'm looking for>
;    git show 7.x-1.2
; 
; ------------------------------------------------
projects[example1][download][revision] = abc1234     ; 7.x-1.0
projects[example2][download][revision] = xyz9876     ; 7.x-3.4
projects[example3][download][revision] = pqr4567     ; 7.x-2.2
