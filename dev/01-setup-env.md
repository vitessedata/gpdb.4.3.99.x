How to Set Up the Dev Environment
===


Setting Up the Profile
---

First, we set up our .profile with some necessary env variables.

```
% cat >> ~/.profile

export TOOLCHAIN_DIR="$HOME/p/toolchain"
export PATH=".:$TOOLCHAIN_DIR/installed/bin:/usr/local/go/bin:$PATH"
export GOPATH=~/p/deepgreen/dg:~/p/toolchain/mendota/phi/go
export EDITOR=vi
ulimit -c unlimited

# do this for ubuntu ... once.
#echo "core.%e.%p" | sudo tee /proc/sys/kernel/core_pattern 

^D
```

Now, logout and login again to effect the .profile settings.


Setting Up the Code Directories
---

As a convention, we put everything under the ~/p directory. 

```
% mkdir ~/p
```

It is now time to pull the code down from github.

```
% cd ~/p
% git clone git@github.com:vitessedata/deepgreen.git
% git clone git@github.com:vitessedata/monona.git
% git clone git@github.com:vitessedata/mendota.git
% git clone git@github.com:vitessedata/madlib.git
% git clone git@github.com:vitessedata/toolchain.git
```

Compiling the Toolchain
---

The toolchain are external libraries we depend on to build Deepgreen DB.

```
% cd ~/p/toolchain
% ln -s ~/p/mendota        # some libraries come from mendota
% bash build.sh
```

Compiling Deepgreen
---

Now, we setup the Deepgreen DB directory for the build. We use 
symbolic links to point to some necessary libraries:

```
% cd ~/p/deepgreen
% # NOTE: YOU ONLY NEED TO DO THE ln -s ONCE 
% ln -s ~/p/toolchain 
% ln -s ~/p/mendota/vdbtools
% ln -s ~/p/madlib
% ln -s ~/p/mendota/phi
```

To build:

```
% cd ~/p/deepgreen
% make config
% make clean
% make -j8  
% make install
```

Initialize the Cluster
---

The following will initialize the cluster for use.

```
cd ~/p/deepgreen/run
source env.sh
gpssh-exkeys -h localhost
bash init2.sh
```
