set $dir=tmp
set $ndirs=20000
set $meandirwidth=150

set mode quit firstdone

define fileset name=removedirsdeep,path=$dir,size=0,leafdirs=$ndirs,dirwidth=$meandirwidth,prealloc

define process name=remdir,instances=1
{
  thread name=removedirectory,memsize=1m,instances=1
  {
    flowop removedir name=dirremover,filesetname=removedirsdeep
  }
}

run