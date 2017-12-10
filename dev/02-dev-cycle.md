Development Cycle
===

Assuming you have set up the environment as described in the previous
section, this is the regular develop / compile / run / crash / debug cycle.

Starting Deepgreen DB
---

The make environment is set up to kill the old DB and start a new one.

```
% cd ~/p/deepgreen
% make run
```

Connecting to the DB
---

Do this once to set up the env variables:

```
% cd ~p/deepgreen/run
% source env.sh
```

Once you have sourced the env.sh file, you should be able to connect
to the server with psql.

```
% psql 
```


