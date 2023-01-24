import os


def judgeFile(filename):
    if filename.startswith("_") or filename.startswith("."):
        return False
    if filename.endswith(".md"):
        return True

    return False


def judgeDir(filepath):
    if filepath.startswith(".\\.git"):
        return False

    return True


def dirList(f, path, space):
    filelist = os.listdir(path)
    for filename in filelist:
        filepath = os.path.join(path, filename)
        space += 4
        if os.path.isdir(filepath) and judgeDir(filepath):
            print(" "*space + "- **"+os.path.basename(filepath)+"**\n", end="")
            f.write(" "*space + "- **"+os.path.basename(filepath)+"**\n")
            dirList(f, filepath, space)
        else:
            filename = os.path.basename(filepath)
            if judgeFile(filename):
                print(" "*space +
                      "- ["+os.path.splitext(os.path.basename(filepath))[0]+"](" + filepath + ")\n", end="")
                f.write(" "*space +
                        "- ["+os.path.splitext(os.path.basename(filepath))[0]+"](" + filepath + ")\n")
        space -= 4


if '__main__':
    with open("_sidebar.md", "w", encoding='UTF-8') as f:
        dirList(f, '.', -4)
