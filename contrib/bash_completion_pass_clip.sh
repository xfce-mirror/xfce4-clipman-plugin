#!/bin/bash
# copied from /usr/share/bash-completion/completions/pass
_pass_clip()
{
  if [[ -z $(typeset -f _pass_complete_entries) ]]
  then
    # load passworstore completion code
    source /usr/share/bash-completion/completions/pass
  fi

	COMPREPLY=()
	local cur="${COMP_WORDS[COMP_CWORD]}"
  COMPREPLY+=($(compgen -- ${cur}))
  _pass_complete_entries 1
}
complete -F _pass_clip pass_clip.sh
