# test1
home
github更新项目子模块
若a项目已经引入了b项目作为子模块，可以使用以下步骤来同步更新b子模块：
进入a项目的根目录，执行以下命令来切换到b子模块的目录：cd b

如果b目录没有更新本地代码：
执行以下命令，将b子模块更新到最新版本：git pull origin main。
这将会从远程仓库中拉取最新版本的代码并自动合并到本地代码。

如果b目录发生了本地更新：
首先要将b目录切换到main分支，然后

git add .
git commit -m "update file"
git push
这将会更新b项目远程的代码

返回到a项目的根目录，执行以下命令将b子模块的更新提交到a项目的仓库：
git add b，然后执行git commit -m "update b submodule"提交更改。
最后，执行git push将更改推送到远程仓库，即可完成同步更新b子模块。
请注意，在执行上述步骤之前，确保在a项目中使用的是b子模块的最新稳定版本。否则可能会引起版本冲突或其他问题。

1、回滚本地提交的记录
git reset --hard 目标commit的hash值
2、强制提交到远端服务器
git push origin HEAD --force

git bash 命令笔记
git branch -a            ：查看所有分支，包括远程和本地
git branch -r            ：查看远程分支
git branch               ：查看本地分支

git checkout 分支名称     ：转换到分支目录


