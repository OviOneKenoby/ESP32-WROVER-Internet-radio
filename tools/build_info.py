Import("env")

import subprocess


def git_identifier():
    try:
        return subprocess.check_output(
            ["git", "rev-parse", "--short=12", "HEAD"],
            cwd=env.subst("$PROJECT_DIR"),
            stderr=subprocess.DEVNULL,
            text=True,
        ).strip() or "unknown"
    except (OSError, subprocess.SubprocessError):
        return "unknown"


env.Append(CPPDEFINES=[("BUILD_GIT_ID", '\\"%s\\"' % git_identifier())])
