set $dir=tmp
set $ndirs=250000
set $meandirwidth=600

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