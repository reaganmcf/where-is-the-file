# AsstLast

## todos
- checkout
- update
- upgrade
- clientside `remove` can't free client_manifest without crash
- clientside commit needs to be error checked and mem checked
- handle pushing empty files
- push() modify doesnt work because manifests have mismatched hashes
- handle push() D on nested dirs


### Protocol

The protocol used between the server and the client is as follows

1) Send length of command
2) Send command name
3) For the rest of the params, send length followed by data

Example:
`14:create_project:10:project1.1`


### Manifest Structure
Manifests are in the following format
```
<project_name>
<version_number>
~ <A/D>:<file_path:name>:<file_current_version>:<file_hash>:!
~ <A/D>:<file_path:name>:<file_current_version>:<file_hash>:
~ <A/D>:<file_path:name>:<file_current_version>:<file_hash>:  
```
If a file entry has `!` at the end, then it means that the server has not seen it.
