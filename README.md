# AsstLast

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
~   <file_path/name>/<file_current_version>/<file_hash>/!
~   <file_path/name>/<file_current_version>/<file_hash>
```

If a file entry has `!` at the end, then it means that the server has not seen it.
The params of each property for each file are separated by `\t`