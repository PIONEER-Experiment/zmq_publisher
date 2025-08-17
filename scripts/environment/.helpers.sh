# .helpers.sh - reusable helper functions for environment detection

# find_root_with_files ROOT1 [ROOT2 ...] -- FILE1 [FILE2 ...] DEBUG
# Finds the first directory under any ROOT that contains all specified FILES (relative paths)
# find_root_with_files ROOT1 [ROOT2 ...] -- FILE1 [FILE2 ...] [DEBUG] [--all]
# Finds directory(ies) under any ROOT that contain all specified FILES (relative paths)
# If --all is passed after files, prints all matches without warnings.
# .helpers.sh - reusable helper functions for environment detection

# find_root_with_files [--all] ROOT1 [ROOT2 ...] -- FILE1 [FILE2 ...] DEBUG
# Finds one or all directories under any ROOT that contain all specified FILES (relative paths)
find_root_with_files() {
  local return_all=false
  local debug="${@: -1}"        # Last arg is debug flag
  set -- "${@:1:$(($#-1))}"     # Remove debug from args

  # Parse optional --all flag
  if [[ "$1" == "--all" ]]; then
    return_all=true
    shift
  fi

  # Split args at '--' to separate roots and required files
  local roots=()
  local files=()
  local in_roots=true

  for arg in "$@"; do
    if [[ "$arg" == "--" ]]; then
      in_roots=false
      continue
    fi
    if $in_roots; then
      roots+=("$arg")
    else
      files+=("$arg")
    fi
  done

  if [[ ${#roots[@]} -eq 0 || ${#files[@]} -eq 0 ]]; then
    echo "ERROR: Usage: find_root_with_files [--all] ROOT1 [ROOT2 ...] -- FILE1 [FILE2 ...] DEBUG" >&2
    return 2
  fi

  local total_searched=0
  local candidates=()

  for root in "${roots[@]}"; do
    $debug && echo "[DEBUG] Searching under: $root for files: ${files[*]}" >&2
    local searched=0
    while IFS= read -r dir; do
      ((searched++))
      local all_found=true
      for relpath in "${files[@]}"; do
        if [[ ! -e "$dir/$relpath" ]]; then
          all_found=false
          break
        fi
      done
      if $all_found; then
        candidates+=("$dir")
        $debug && echo "[DEBUG] Match found: $dir" >&2
        if ! $return_all; then
          echo "$dir"
          return 0
        fi
      fi
    done < <(find "$root" -type d 2>/dev/null)
    ((total_searched+=searched))
  done

  $debug && echo "[DEBUG] Total directories searched: $total_searched" >&2
  $debug && echo "[DEBUG] Candidates found: ${#candidates[@]}" >&2

  if [[ ${#candidates[@]} -eq 0 ]]; then
    echo "ERROR: No directory found containing: ${files[*]}" >&2
    return 1
  fi

  if $return_all; then
    printf "%s\n" "${candidates[@]}"
  else
    if [[ ${#candidates[@]} -gt 1 ]]; then
      echo "WARNING: Multiple directories found. Using first match: ${candidates[0]}" >&2
    fi
    echo "${candidates[0]}"
  fi

  return 0
}




# find_file_in_candidates CANDIDATE_DIRS... -- FILENAME DEBUG
# Search recursively under each candidate directory for the given file,
# return first found path (or print multiple candidates and pick first)
find_file_in_candidates() {
  local debug="${@: -1}"  # last argument is debug flag
  set -- "${@:1:$(($#-1))}" # rest are dirs + filename split below

  local dirs=()
  local filename=""
  local in_dirs=true

  for arg in "$@"; do
    if $in_dirs; then
      if [[ "$arg" == "--" ]]; then
        in_dirs=false
      else
        dirs+=("$arg")
      fi
    else
      filename="$arg"
    fi
  done

  if [[ -z "$filename" || ${#dirs[@]} -eq 0 ]]; then
    echo "ERROR: Usage: find_file_in_candidates DIR1 [DIR2 ...] -- FILENAME DEBUG" >&2
    return 2
  fi

  local candidates=()
  for dir in "${dirs[@]}"; do
    $debug && echo "[DEBUG] Searching for '$filename' under $dir ..." >&2
    while IFS= read -r file; do
      candidates+=("$file")
    done < <(find "$dir" -type f -name "$filename" 2>/dev/null)
  done

  if [[ ${#candidates[@]} -eq 0 ]]; then
    $debug && echo "[DEBUG] No '$filename' file found in candidates" >&2
    return 1
  fi

  if [[ ${#candidates[@]} -gt 1 ]]; then
    echo "WARNING: Multiple candidates for '$filename' found:" >&2
    for c in "${candidates[@]}"; do
      echo "  $c" >&2
    done
    echo "Using the first one: ${candidates[0]}" >&2
  fi

  echo "${candidates[0]}"
  return 0
}


