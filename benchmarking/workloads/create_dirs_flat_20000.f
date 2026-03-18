set $dir=tmp
set $ndirs=20000
set $meandirwidth=150

set mode quit firstdone

define fileset name=createdirsflat,path=$dir,size=0,leafdirs=$ndirs,dirwidth=$meandirwidth

define process name=dirmake,instances=1
{
  thread name=dirmaker,memsize=1m,instances=1
  {
    flowop makedir name=mkdir1,filesetname=createdirsflat
  }
}

run