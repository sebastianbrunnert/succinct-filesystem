set $dir=tmp
set $ndirs=100000
set $meandirwidth=200

set mode quit firstdone

define fileset name=createdirsdeep,path=$dir,size=0,leafdirs=$ndirs,dirwidth=$meandirwidth

define process name=dirmake,instances=1
{
  thread name=dirmaker,memsize=1m,instances=1
  {
    flowop makedir name=mkdir1,filesetname=createdirsdeep
  }
}

run