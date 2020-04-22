# AsstLast

### Protocol

The protocol used between the server and the client is as follows

1) Send length of command
2) Send command name
3) For the rest of the params, send length followed by data

Example:
`14:create_project:10:project1.1`