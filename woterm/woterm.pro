

TEMPLATE=subdirs #表示要编译多个子目录中的工程
CONFIG += ordered #要求各个子工程按顺序编译

SUBDIRS += qtermwidget woterm #指定子工程的编译顺序；先编译库，再编译可执行文件
