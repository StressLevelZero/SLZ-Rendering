# SLZ Rendering
Fork of Unity's Graphics repo containing all of the SRP related packages.

# For Stress Level Zero Maintainers
## Syncing with Unity Graphics Repo
We use git rebase to append our changes on top of unity's graphics repo. Before rebasing, always make sure that no one else has pending changes that need to be pushed, and that you have pulled down the latest changes! Also, I suggest making a backup of the current branch as a rebase is very hard to revert if something goes wrong.

To rebase, first commit any pending changes and pull. If you have not already done so, add unity's graphics repo as the remote `upstream`:
```
git remote add upstream https://github.com/Unity-Technologies/Graphics.git
```
Determine what branch you should be rebasing on. Usually, this is a branch of the name `XXXX.X/staging` where `XXXX.X` is the unity version. Fetch the latest changes for that branch:
```
git fetch upstream XXXX.X/staging
```
If you are rebasing on the same upstream branch that this repo is currently based on, run the following:
```
git rebase upstream/XXXX.X/staging
```
If you are rebasing on a different branch (or are unsure what the current repo is based on), you must find the commit hash of the __parent__ of the first commit. This can be done by searching for the commit message of the first commit:
```
git log --pretty=format:"%P" --grep='Initial commit, added URP config package generator from old URP fork'
```
With the commit hash of the __parent__ of the first commit, run the following. This will only rebase the commits after our first commit:
```
git rebase --onto upstream/XXXX.X/staging FULL_40_CHAR_COMMIT_HASH_OF_PARENT
```
You will probably run into merge conflicts. Resolve those, and continue until it completes. Finally, either push the new history to a new branch or use `git push --force` to overwrite the history of the old branch. If you want to overwrite, make sure you have a backup of the branch first!


## Tips

In order to work on the packages contained in this repo, I suggest cloning this repo in a folder outside of your unity project(s), then making symbolic links from the individual packages to your project's \Packages folder.

Avoid making changes to files added by Unity as much as possible. Significant changes will probably result in hard to resolve merge conflicts down the line unless they are trivial. When you do, mark where changes begin with `/// SLZ MODIFIED - (Reason for modification)` and where they end with `/// END SLZ MODIFIED` so others and your future self know what we've modified and why. To remove unity code, use C89 style `/* */` comments on new lines or preprocessor `#if` guards rather than C++ `//` as that counts as deletion and addition of new lines.

Keep new files quarantined to SLZ folders. This makes it much easier to keep track of our additions. 

If you want to move a file, use `git mv` to ensure git sees the file as moved rather than as a deletion and a new file. If you want to completely replace a file added by unity, move it to a new filename so git does not think the history of the old file belongs to your new version.

# TODO
