# MSGDIST
MSGDIST is a message management and redistribution system made in the C language.


1. makefile
2. run gestor executing command: ./gestor
3. run clients executing command: ./cliente [name]


Gestor commands:
  filtro on    - activate filter on created messages
  filtro off   - desactivate filter on created messages
  users        - list of users
  topics       - list of topics
  msg          - list of all topics and messages
  topic [name] - list all messages of topic [name]
  del [msg]    - delete message [msg]
  kick [user]  - kick user [user]
  shutdown     - shutdown the system
  
 Cliente commands:
  topics         - list of topics
  titles [topic] - list of existing titles of topic [topic]
  show [title]   - show content of message with title [title]
  sub [topic]    - subscribe to topic [topic]
  unsub [topic]  - unsubscribe topic [topic]
  show_subs      - list of topics subs
  msg            - create new message
  logout         - close 

See an example: https://youtu.be/GjRX0WGMefQ 
