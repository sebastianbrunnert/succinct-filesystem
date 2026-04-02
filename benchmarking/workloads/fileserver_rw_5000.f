set $dir=tmp
set $nfiles=5000
set $meandirwidth=20
set $meanfilesize=128k
set $iosize=1m
set $meanappendsize=16k

define fileset name=fileserver5000,path=$dir,size=$meanfilesize,entries=$nfiles,dirwidth=$meandirwidth,prealloc=80

define process name=filereader,instances=1
{
    thread name=filereaderthread,memsize=10m,instances=1
    {
        flowop createfile name=createfile1,filesetname=fileserver5000,fd=1
        flowop writewholefile name=wrtfile1,srcfd=1,fd=1,iosize=$iosize
        flowop closefile name=closefile1,fd=1
        flowop openfile name=openfile1,filesetname=fileserver5000,fd=1
        flowop appendfilerand name=appendfilerand1,iosize=$meanappendsize,fd=1
        flowop closefile name=closefile2,fd=1
        flowop openfile name=openfile2,filesetname=fileserver5000,fd=1
        flowop readwholefile name=readfile1,fd=1,iosize=$iosize
        flowop closefile name=closefile3,fd=1
        flowop deletefile name=deletefile1,filesetname=fileserver5000
        flowop statfile name=statfile1,filesetname=fileserver5000
    }
}

run 10