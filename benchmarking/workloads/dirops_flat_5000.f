set $dir=tmp
set $ndirs=10000
set $meandirwidth=150

define fileset name=diropsflat,path=$dir,size=0,leafdirs=$ndirs,dirwidth=$meandirwidth,prealloc=50

define process name=dirops,instances=1
{
  thread name=dirworker,memsize=1m,instances=$1
  {
    flowop makedir name=mkdir1,filesetname=diropsflat
    flowop listdir name=listdir1,filesetname=diropsflat
    flowop removedir name=rmdir1,filesetname=diropsflat
  }
}

run 10