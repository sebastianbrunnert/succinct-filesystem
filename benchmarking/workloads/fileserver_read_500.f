set $dir=tmp
set $nfiles=500
set $meandirwidth=20
set $meanfilesize=128k

define fileset name=fileserver500,path=$dir,size=$meanfilesize,entries=$nfiles,dirwidth=$meandirwidth,prealloc

define process name=filereader,instances=1
{
    thread name=filereaderthread,memsize=10m,instances=1
    {
        flowop openfile name=openfile1, filesetname=fileserver500, fd=1
        flowop readwholefile name=readfile1, fd=1
        flowop closefile name=closefile1, fd=1
        flowop statfile name=statfile1, filesetname=fileserver500
    }
}

run 10