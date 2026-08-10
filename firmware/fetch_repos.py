import os
import subprocess
import json


def clone_or_update_repo(
    repo_url, path, ref=None, with_submodules=False, patch_paths=None
):
    import os

    if not os.path.exists(path):
        subprocess.run(["git", "clone", repo_url, path], check=True)
    else:
        subprocess.run(["git", "-C", path, "fetch"], check=True)

    if ref:
        subprocess.run(["git", "-C", path, "checkout", ref], check=True)

    if with_submodules:
        subprocess.run(
            ["git", "-C", path, "submodule", "update", "--init", "--recursive"],
            check=True,
        )

    # Apply local patches. Some upstream tags differ only in whitespace or final
    # newlines, so retry with Git's whitespace-tolerant mode before giving up.
    for patch_path in patch_paths or []:
        patch_full_path = (
            patch_path
            if os.path.isabs(patch_path)
            else os.path.join(os.getcwd(), patch_path)
        )
        # 使用 git apply --check 先检测补丁是否能应用，避免报错
        apply_args = ["git", "-C", path, "apply"]
        check_result = subprocess.run(
            apply_args + ["--check", patch_full_path],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        if check_result.returncode != 0:
            apply_args.extend(["--ignore-space-change", "--ignore-whitespace"])
            check_result = subprocess.run(
                apply_args + ["--check", patch_full_path],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )

        if check_result.returncode == 0:
            subprocess.run(apply_args + [patch_full_path], check=True)
            print(f"Applied patch {patch_path} to {path}")
        else:
            reverse_check = subprocess.run(
                apply_args + ["--reverse", "--check", patch_full_path],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            if reverse_check.returncode == 0:
                print(f"Patch {patch_path} is already applied to {path}")
            else:
                raise RuntimeError(
                    f"Patch {patch_path} cannot be applied cleanly to {path}"
                )


def fetch_dependencies():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    config_path = os.path.join(script_dir, "repos.json")

    with open(config_path) as f:
        repos = json.load(f)

    for repo in repos:
        repo_path = os.path.join(script_dir, repo["path"])
        branch = repo.get("branch")
        with_submodules = repo.get("with_submodules", False)
        patches = repo.get("patches")
        if patches is None:
            patches = [repo["patch"]] if repo.get("patch") else []
        patches = [
            patch if os.path.isabs(patch) else os.path.join(script_dir, patch)
            for patch in patches
        ]
        clone_or_update_repo(
            repo["url"], repo_path, branch, with_submodules, patches
        )


if __name__ == "__main__":
    fetch_dependencies()
