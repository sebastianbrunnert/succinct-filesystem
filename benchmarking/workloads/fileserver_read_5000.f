set $dir=tmp
set $nfiles=5000
set $meandirwidth=20
set $meanfilesize=128k

define fileset name=fileserver5000,path=$dir,size=$meanfilesize,entries=$nfiles,dirwidth=$meandirwidth,prealloc

define process name=filereader,instances=1
{
    thread name=filereaderthread,memsize=10m,instances=1
    {
        flowop openfile name=openfile1, filesetname=fileserver5000, fd=1
        flowop readwholefile name=readfile1, fd=1
        flowop closefile name=closefile1, fd=1
        flowop statfile name=statfile1, filesetname=fileserver5000
    }
}

run 10