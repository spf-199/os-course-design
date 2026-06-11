# 操作系统课程设计项目

本项目按《操作系统》课程设计要求完成“基础必做 + 自由扩展提升”两级内容。

基础必做部分：

1. 处理机调度：FCFS、SJF、RR、优先级调度。
2. 内存管理：动态分区首次适应、最佳适应；页面置换 FIFO、LRU。
3. 进程同步与并发控制：生产者消费者、读者写者、哲学家进餐。
4. 文件系统：目录项、文件创建/读写/删除、位图空闲块管理。

自由扩展提升部分：

Linux 内核与系统编程方向，实现 `/proc/oslab_monitor` 内核模块，并配套用户态访问程序 `proc_client`。
该模块已针对较老的 Linux 4.4 内核和较新的 Linux 5.x/6.x 内核做条件编译兼容：
Linux 4.11 以下不包含 `linux/sched/signal.h`，Linux 5.6 以下使用 `struct file_operations`，
Linux 5.6 及以上使用 `struct proc_ops`。

## 目录结构

```text
os-course-design/
  include/oslab.h              公共接口
  src/main.c                   主菜单与命令行入口
  src/scheduler.c              处理机调度实验
  src/memory.c                 内存管理实验
  src/sync.c                   进程同步实验
  src/filesystem.c             模拟文件系统实验
  src/proc_client.c            /proc 扩展用户态测试工具
  kernel/oslab_proc.c          Linux procfs 内核模块
  kernel/Makefile              内核模块构建文件
  tests/run_tests.sh           Linux 自动化测试脚本
  tests/run_kernel_smoke.sh    Linux 内核模块烟雾测试脚本
  docs/202330551312_申佩凡_OS课程设计.docx  可编辑 Word 报告
  docs/202330551312_申佩凡_OS课程设计.pdf   最终 PDF 报告
```

## Linux 编译与运行

安装工具：

```bash
sudo apt update
sudo apt install -y build-essential linux-headers-$(uname -r)
```

编译用户态程序：

```bash
make
```

运行一键演示：

```bash
./oslab --demo
```

运行自检：

```bash
./oslab --test
```

进入交互菜单：

```bash
./oslab
```

## Linux 内核模块扩展运行步骤

编译内核模块：

```bash
make kernel
```

如果虚拟机内核较老，例如 `4.4.0-142-generic`，本项目仍可编译。若仍报错，请先确认内核头文件已安装：

```bash
sudo apt install -y build-essential linux-headers-$(uname -r)
ls /lib/modules/$(uname -r)/build
```

加载模块：

```bash
sudo insmod kernel/oslab_proc.ko
```

查看接口输出：

```bash
cat /proc/oslab_monitor
./proc_client
```

写入控制命令：

```bash
echo "note=course_design_test" | sudo tee /proc/oslab_monitor
cat /proc/oslab_monitor
echo "reset" | sudo tee /proc/oslab_monitor
```

卸载模块：

```bash
sudo rmmod oslab_proc
```

查看内核日志：

```bash
dmesg | tail -n 20
```

一键验证内核模块扩展：

```bash
sudo bash tests/run_kernel_smoke.sh
```

## Windows 本机快速验证

如果本机有 MinGW GCC，可以只验证用户态基础模块：

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 -Iinclude -o oslab.exe src/main.c src/scheduler.c src/memory.c src/sync.c src/filesystem.c
.\oslab.exe --test
.\oslab.exe --demo
```

说明：Windows 环境没有 POSIX `pthread`/`semaphore` 与 Linux 内核构建环境，所以同步实验会提示在 Linux 下运行，内核模块也必须在 Linux 下编译加载。

## 代码 URL 填写说明

课程要求报告必须包含代码 URL 和访问方式说明。为了避免伪造地址，本项目没有预填不存在的 GitHub 仓库。提交前请将本目录上传到 GitHub 或 Gitee，然后把真实仓库地址填入 `docs/202330551312_申佩凡_OS课程设计.docx` 的“代码 URL 与访问方式”一节，并重新导出 PDF。

示例上传命令：

```bash
git init
git add .
git commit -m "complete os course design"
git branch -M main
git remote add origin https://github.com/your-account/os-course-design.git
git push -u origin main
```
