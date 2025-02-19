# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generic presubmit checks that can be reused by other presubmit checks."""

from __future__ import print_function

import os as _os

from warnings import warn
_HERE = _os.path.dirname(_os.path.abspath(__file__))

# These filters will be disabled if callers do not explicitly supply a
# (possibly-empty) list.  Ideally over time this list could be driven to zero.
# TODO(pkasting): If some of these look like "should never enable", move them
# to OFF_UNLESS_MANUALLY_ENABLED_LINT_FILTERS.
#
# Justifications for each filter:
#
# - build/include       : Too many; fix in the future
#                         TODO(pkasting): Try enabling subcategories
# - build/include_order : Not happening; #ifdefed includes
# - build/namespaces    : TODO(pkasting): Try re-enabling
# - readability/casting : Mistakes a whole bunch of function pointers
# - runtime/int         : Can be fixed long term; volume of errors too high
# - whitespace/braces   : We have a lot of explicit scoping in chrome code
OFF_BY_DEFAULT_LINT_FILTERS = [
  '-build/include',
  '-build/include_order',
  '-build/namespaces',
  '-readability/casting',
  '-runtime/int',
  '-whitespace/braces',
]

# These filters will be disabled unless callers explicitly enable them, because
# they are undesirable in some way.
#
# Justifications for each filter:
# - build/c++11         : Include file and feature blocklists are
#                         google3-specific
# - runtime/references  : No longer banned by Google style guide
OFF_UNLESS_MANUALLY_ENABLED_LINT_FILTERS = [
  '-build/c++11',
  '-runtime/references',
]

### Description checks

def CheckChangeHasBugField(input_api, output_api):
  """Requires that the changelist have a Bug: field."""
  if input_api.change.BugsFromDescription():
    return []
  else:
    return [output_api.PresubmitNotifyResult(
        'If this change has an associated bug, add Bug: [bug number].')]

def CheckChangeHasNoUnwantedTags(input_api, output_api):
  UNWANTED_TAGS = {
      'FIXED': {
          'why': 'is not supported',
          'instead': 'Use "Fixed:" instead.'
      },
      # TODO: BUG, ISSUE
  }

  errors = []
  for tag, desc in UNWANTED_TAGS.items():
    if tag in input_api.change.tags:
      subs = tag, desc['why'], desc.get('instead', '')
      errors.append(('%s= %s. %s' % subs).rstrip())

  return [output_api.PresubmitError('\n'.join(errors))] if errors else []

def CheckDoNotSubmitInDescription(input_api, output_api):
  """Checks that the user didn't add 'DO NOT ''SUBMIT' to the CL description.
  """
  keyword = 'DO NOT ''SUBMIT'
  if keyword in input_api.change.DescriptionText():
    return [output_api.PresubmitError(
        keyword + ' is present in the changelist description.')]
  else:
    return []


def CheckChangeHasDescription(input_api, output_api):
  """Checks the CL description is not empty."""
  text = input_api.change.DescriptionText()
  if text.strip() == '':
    if input_api.is_committing:
      return [output_api.PresubmitError('Add a description to the CL.')]
    else:
      return [output_api.PresubmitNotifyResult('Add a description to the CL.')]
  return []


def CheckChangeWasUploaded(input_api, output_api):
  """Checks that the issue was uploaded before committing."""
  if input_api.is_committing and not input_api.change.issue:
    return [output_api.PresubmitError(
      'Issue wasn\'t uploaded. Please upload first.')]
  return []


### Content checks


def CheckAuthorizedAuthor(input_api, output_api, bot_allowlist=None):
  """For non-googler/chromites committers, verify the author's email address is
  in AUTHORS.
  """
  if input_api.is_committing:
    error_type = output_api.PresubmitError
  else:
    error_type = output_api.PresubmitPromptWarning

  author = input_api.change.author_email
  if not author:
    input_api.logging.info('No author, skipping AUTHOR check')
    return []

  # This is used for CLs created by trusted robot accounts.
  if bot_allowlist and author in bot_allowlist:
    return []

  authors_path = input_api.os_path.join(
      input_api.PresubmitLocalPath(), 'AUTHORS')
  author_re = input_api.re.compile(r'[^#]+\s+\<(.+?)\>\s*$')
  valid_authors = []
  with open(authors_path, 'rb') as fp:
    for line in fp:
      m = author_re.match(line.decode('utf8'))
      if m:
        valid_authors.append(m.group(1).lower())

  if not any(input_api.fnmatch.fnmatch(author.lower(), valid)
             for valid in valid_authors):
    input_api.logging.info('Valid authors are %s', ', '.join(valid_authors))
    return [error_type(
        ('%s is not in AUTHORS file. If you are a new contributor, please visit'
        '\n'
        'https://www.chromium.org/developers/contributing-code and read the '
        '"Legal" section\n'
        'If you are a chromite, verify the contributor signed the CLA.') %
        author)]
  return []


def CheckDoNotSubmitInFiles(input_api, output_api):
  """Checks that the user didn't add 'DO NOT ''SUBMIT' to any files."""
  # We want to check every text file, not just source files.
  file_filter = lambda x : x
  keyword = 'DO NOT ''SUBMIT'
  def DoNotSubmitRule(extension, line):
    try:
      return keyword not in line
    # Fallback to True for non-text content
    except UnicodeDecodeError:
      return True

  errors = _FindNewViolationsOfRule(DoNotSubmitRule, input_api, file_filter)
  text = '\n'.join('Found %s in %s' % (keyword, loc) for loc in errors)
  if text:
    return [output_api.PresubmitError(text)]
  return []


def GetCppLintFilters(lint_filters=None):
  filters = OFF_UNLESS_MANUALLY_ENABLED_LINT_FILTERS[:]
  if lint_filters is None:
    lint_filters = OFF_BY_DEFAULT_LINT_FILTERS
  filters.extend(lint_filters)
  return filters


def CheckChangeLintsClean(input_api, output_api, source_file_filter=None,
                          lint_filters=None, verbose_level=None):
  """Checks that all '.cc' and '.h' files pass cpplint.py."""
  _RE_IS_TEST = input_api.re.compile(r'.*tests?.(cc|h)$')
  result = []

  cpplint = input_api.cpplint
  # Access to a protected member _XX of a client class
  # pylint: disable=protected-access
  cpplint._cpplint_state.ResetErrorCounts()

  cpplint._SetFilters(','.join(GetCppLintFilters(lint_filters)))

  # We currently are more strict with normal code than unit tests; 4 and 5 are
  # the verbosity level that would normally be passed to cpplint.py through
  # --verbose=#. Hopefully, in the future, we can be more verbose.
  files = [f.AbsoluteLocalPath() for f in
           input_api.AffectedSourceFiles(source_file_filter)]
  for file_name in files:
    if _RE_IS_TEST.match(file_name):
      level = 5
    else:
      level = 4

    verbose_level = verbose_level or level
    cpplint.ProcessFile(file_name, verbose_level)

  if cpplint._cpplint_state.error_count > 0:
    if input_api.is_committing:
      res_type = output_api.PresubmitError
    else:
      res_type = output_api.PresubmitPromptWarning
    result = [res_type('Changelist failed cpplint.py check.')]

  return result


def CheckChangeHasNoCR(input_api, output_api, source_file_filter=None):
  """Checks no '\r' (CR) character is in any source files."""
  cr_files = []
  for f in input_api.AffectedSourceFiles(source_file_filter):
    if '\r' in input_api.ReadFile(f, 'rb'):
      cr_files.append(f.LocalPath())
  if cr_files:
    return [output_api.PresubmitPromptWarning(
        'Found a CR character in these files:', items=cr_files)]
  return []


def CheckChangeHasOnlyOneEol(input_api, output_api, source_file_filter=None):
  """Checks the files ends with one and only one \n (LF)."""
  eof_files = []
  for f in input_api.AffectedSourceFiles(source_file_filter):
    contents = input_api.ReadFile(f, 'rb')
    # Check that the file ends in one and only one newline character.
    if len(contents) > 1 and (contents[-1:] != '\n' or contents[-2:-1] == '\n'):
      eof_files.append(f.LocalPath())

  if eof_files:
    return [output_api.PresubmitPromptWarning(
      'These files should end in one (and only one) newline character:',
      items=eof_files)]
  return []


def CheckChangeHasNoCrAndHasOnlyOneEol(input_api, output_api,
                                       source_file_filter=None):
  """Runs both CheckChangeHasNoCR and CheckChangeHasOnlyOneEOL in one pass.

  It is faster because it is reading the file only once.
  """
  cr_files = []
  eof_files = []
  for f in input_api.AffectedSourceFiles(source_file_filter):
    contents = input_api.ReadFile(f, 'rb')
    if '\r' in contents:
      cr_files.append(f.LocalPath())
    # Check that the file ends in one and only one newline character.
    if len(contents) > 1 and (contents[-1:] != '\n' or contents[-2:-1] == '\n'):
      eof_files.append(f.LocalPath())
  outputs = []
  if cr_files:
    outputs.append(output_api.PresubmitPromptWarning(
        'Found a CR character in these files:', items=cr_files))
  if eof_files:
    outputs.append(output_api.PresubmitPromptWarning(
      'These files should end in one (and only one) newline character:',
      items=eof_files))
  return outputs

def CheckGenderNeutral(input_api, output_api, source_file_filter=None):
  """Checks that there are no gendered pronouns in any of the text files to be
  submitted.
  """
  gendered_re = input_api.re.compile(
      r'(^|\s|\(|\[)([Hh]e|[Hh]is|[Hh]ers?|[Hh]im|[Ss]he|[Gg]uys?)\\b')

  errors = []
  for f in input_api.AffectedFiles(include_deletes=False,
                                   file_filter=source_file_filter):
    for line_num, line in f.ChangedContents():
      if gendered_re.search(line):
        errors.append('%s (%d): %s' % (f.LocalPath(), line_num, line))

  if len(errors):
    return [output_api.PresubmitPromptWarning('Found a gendered pronoun in:',
                                              long_text='\n'.join(errors))]
  return []



def _ReportErrorFileAndLine(filename, line_num, dummy_line):
  """Default error formatter for _FindNewViolationsOfRule."""
  return '%s:%s' % (filename, line_num)


def _GenerateAffectedFileExtList(input_api, source_file_filter):
  """Generate a list of (file, extension) tuples from affected files.

  The result can be fed to _FindNewViolationsOfRule() directly, or
  could be filtered before doing that.

  Args:
    input_api: object to enumerate the affected files.
    source_file_filter: a filter to be passed to the input api.
  Yields:
    A list of (file, extension) tuples, where |file| is an affected
      file, and |extension| its file path extension.
  """
  for f in input_api.AffectedFiles(
      include_deletes=False, file_filter=source_file_filter):
    extension = str(f.LocalPath()).rsplit('.', 1)[-1]
    yield (f, extension)


def _FindNewViolationsOfRuleForList(callable_rule,
                                    file_ext_list,
                                    error_formatter=_ReportErrorFileAndLine):
  """Find all newly introduced violations of a per-line rule (a callable).

  Prefer calling _FindNewViolationsOfRule() instead of this function, unless
  the list of affected files need to be filtered in a special way.

  Arguments:
    callable_rule: a callable taking a file extension and line of input and
      returning True if the rule is satisfied and False if there was a problem.
    file_ext_list: a list of input (file, extension) tuples, as returned by
      _GenerateAffectedFileExtList().
    error_formatter: a callable taking (filename, line_number, line) and
      returning a formatted error string.

  Returns:
    A list of the newly-introduced violations reported by the rule.
  """
  errors = []
  for f, extension in file_ext_list:
    # For speed, we do two passes, checking first the full file.  Shelling out
    # to the SCM to determine the changed region can be quite expensive on
    # Win32.  Assuming that most files will be kept problem-free, we can
    # skip the SCM operations most of the time.
    if all(callable_rule(extension, line) for line in f.NewContents()):
      continue  # No violation found in full text: can skip considering diff.

    for line_num, line in f.ChangedContents():
      if not callable_rule(extension, line):
        errors.append(error_formatter(f.LocalPath(), line_num, line))

  return errors


def _FindNewViolationsOfRule(callable_rule,
                             input_api,
                             source_file_filter=None,
                             error_formatter=_ReportErrorFileAndLine):
  """Find all newly introduced violations of a per-line rule (a callable).

  Arguments:
    callable_rule: a callable taking a file extension and line of input and
      returning True if the rule is satisfied and False if there was a problem.
    input_api: object to enumerate the affected files.
    source_file_filter: a filter to be passed to the input api.
    error_formatter: a callable taking (filename, line_number, line) and
      returning a formatted error string.

  Returns:
    A list of the newly-introduced violations reported by the rule.
  """
  return _FindNewViolationsOfRuleForList(
      callable_rule, _GenerateAffectedFileExtList(
          input_api, source_file_filter), error_formatter)


def CheckChangeHasNoTabs(input_api, output_api, source_file_filter=None):
  """Checks that there are no tab characters in any of the text files to be
  submitted.
  """
  # In addition to the filter, make sure that makefiles are skipped.
  if not source_file_filter:
    # It's the default filter.
    source_file_filter = input_api.FilterSourceFile
  def filter_more(affected_file):
    basename = input_api.os_path.basename(affected_file.LocalPath())
    return (not (basename in ('Makefile', 'makefile') or
                 basename.endswith('.mk')) and
            source_file_filter(affected_file))

  tabs = _FindNewViolationsOfRule(lambda _, line : '\t' not in line,
                                  input_api, filter_more)

  if tabs:
    return [output_api.PresubmitPromptWarning('Found a tab character in:',
                                              long_text='\n'.join(tabs))]
  return []


def CheckChangeTodoHasOwner(input_api, output_api, source_file_filter=None):
  """Checks that the user didn't add TODO(name) without an owner."""

  unowned_todo = input_api.re.compile('TO''DO[^(]')
  errors = _FindNewViolationsOfRule(lambda _, x : not unowned_todo.search(x),
                                    input_api, source_file_filter)
  errors = ['Found TO''DO with no owner in ' + x for x in errors]
  if errors:
    return [output_api.PresubmitPromptWarning('\n'.join(errors))]
  return []


def CheckChangeHasNoStrayWhitespace(input_api, output_api,
                                    source_file_filter=None):
  """Checks that there is no stray whitespace at source lines end."""
  errors = _FindNewViolationsOfRule(lambda _, line : line.rstrip() == line,
                                    input_api, source_file_filter)
  if errors:
    return [output_api.PresubmitPromptWarning(
        'Found line ending with white spaces in:',
        long_text='\n'.join(errors))]
  return []


def CheckLongLines(input_api, output_api, maxlen, source_file_filter=None):
  """Checks that there aren't any lines longer than maxlen characters in any of
  the text files to be submitted.
  """
  maxlens = {
      'java': 100,
      # This is specifically for Android's handwritten makefiles (Android.mk).
      'mk': 200,
      '': maxlen,
  }

  # Language specific exceptions to max line length.
  # '.h' is considered an obj-c file extension, since OBJC_EXCEPTIONS are a
  # superset of CPP_EXCEPTIONS.
  CPP_FILE_EXTS = ('c', 'cc')
  CPP_EXCEPTIONS = ('#define', '#endif', '#if', '#include', '#pragma')
  HTML_FILE_EXTS = ('html',)
  HTML_EXCEPTIONS = ('<g ', '<link ', '<path ',)
  JAVA_FILE_EXTS = ('java',)
  JAVA_EXCEPTIONS = ('import ', 'package ')
  JS_FILE_EXTS = ('js',)
  JS_EXCEPTIONS = ("GEN('#include", 'import ')
  TS_FILE_EXTS = ('ts',)
  TS_EXCEPTIONS = ('import ')
  OBJC_FILE_EXTS = ('h', 'm', 'mm')
  OBJC_EXCEPTIONS = ('#define', '#endif', '#if', '#import', '#include',
                     '#pragma')
  PY_FILE_EXTS = ('py',)
  PY_EXCEPTIONS = ('import', 'from')

  LANGUAGE_EXCEPTIONS = [
    (CPP_FILE_EXTS, CPP_EXCEPTIONS),
    (HTML_FILE_EXTS, HTML_EXCEPTIONS),
    (JAVA_FILE_EXTS, JAVA_EXCEPTIONS),
    (JS_FILE_EXTS, JS_EXCEPTIONS),
    (TS_FILE_EXTS, TS_EXCEPTIONS),
    (OBJC_FILE_EXTS, OBJC_EXCEPTIONS),
    (PY_FILE_EXTS, PY_EXCEPTIONS),
  ]

  def no_long_lines(file_extension, line):
    # Check for language specific exceptions.
    if any(file_extension in exts and line.lstrip().startswith(exceptions)
           for exts, exceptions in LANGUAGE_EXCEPTIONS):
      return True

    file_maxlen = maxlens.get(file_extension, maxlens[''])
    # Stupidly long symbols that needs to be worked around if takes 66% of line.
    long_symbol = file_maxlen * 2 / 3
    # Hard line length limit at 50% more.
    extra_maxlen = file_maxlen * 3 / 2

    line_len = len(line)
    if line_len <= file_maxlen:
      return True

    # Allow long URLs of any length.
    if any((url in line) for url in ('file://', 'http://', 'https://')):
      return True

    if line_len > extra_maxlen:
      return False

    if 'url(' in line and file_extension == 'css':
      return True

    if '<include' in line and file_extension in ('css', 'html', 'js'):
      return True

    return input_api.re.match(
        r'.*[A-Za-z][A-Za-z_0-9]{%d,}.*' % long_symbol, line)

  def is_global_pylint_directive(line, pos):
    """True iff the pylint directive starting at line[pos] is global."""
    # Any character before |pos| that is not whitespace or '#' indidcates
    # this is a local directive.
    return not any([c not in " \t#" for c in line[:pos]])

  def check_python_long_lines(affected_files, error_formatter):
    errors = []
    global_check_enabled = True

    for f in affected_files:
      file_path = f.LocalPath()
      for idx, line in enumerate(f.NewContents()):
        line_num = idx + 1
        line_is_short = no_long_lines(PY_FILE_EXTS[0], line)

        pos = line.find('pylint: disable=line-too-long')
        if pos >= 0:
          if is_global_pylint_directive(line, pos):
            global_check_enabled = False  # Global disable
          else:
            continue  # Local disable.

        do_check = global_check_enabled

        pos = line.find('pylint: enable=line-too-long')
        if pos >= 0:
          if is_global_pylint_directive(line, pos):
            global_check_enabled = True  # Global enable
            do_check = True  # Ensure it applies to current line as well.
          else:
            do_check = True  # Local enable

        if do_check and not line_is_short:
          errors.append(error_formatter(file_path, line_num, line))

    return errors

  def format_error(filename, line_num, line):
    return '%s, line %s, %s chars' % (filename, line_num, len(line))

  file_ext_list = list(
      _GenerateAffectedFileExtList(input_api, source_file_filter))

  errors = []

  # For non-Python files, a simple line-based rule check is enough.
  non_py_file_ext_list = [x for x in file_ext_list if x[1] not in PY_FILE_EXTS]
  if non_py_file_ext_list:
    errors += _FindNewViolationsOfRuleForList(
        no_long_lines, non_py_file_ext_list, error_formatter=format_error)

  # However, Python files need more sophisticated checks that need parsing
  # the whole source file.
  py_file_list = [x[0] for x in file_ext_list if x[1] in PY_FILE_EXTS]
  if py_file_list:
    errors += check_python_long_lines(
        py_file_list, error_formatter=format_error)
  if errors:
    msg = 'Found lines longer than %s characters (first 5 shown).' % maxlen
    return [output_api.PresubmitPromptWarning(msg, items=errors[:5])]
  else:
    return []


def CheckLicense(input_api, output_api, license_re=None, project_name=None,
    source_file_filter=None, accept_empty_files=True):
  """Verifies the license header.
  """

  project_name = project_name or 'Chromium'

  # Accept any year number from 2006 to the current year, or the special
  # 2006-20xx string used on the oldest files. 2006-20xx is deprecated, but
  # tolerated on old files.
  current_year = int(input_api.time.strftime('%Y'))
  allowed_years = (str(s) for s in reversed(range(2006, current_year + 1)))
  years_re = '(' + '|'.join(allowed_years) + '|2006-2008|2006-2009|2006-2010)'

  # The (c) is deprecated, but tolerate it until it's removed from all files.
  license_re = license_re or (
      r'.*? Copyright (\(c\) )?%(year)s The %(project)s Authors\. '
        r'All rights reserved\.\n'
      r'.*? Use of this source code is governed by a BSD-style license that '
        r'can be\n'
      r'.*? found in the LICENSE file\.(?: \*/)?\n'
  ) % {
      'year': years_re,
      'project': project_name,
  }

  license_re = input_api.re.compile(license_re, input_api.re.MULTILINE)
  bad_files = []
  for f in input_api.AffectedSourceFiles(source_file_filter):
    contents = input_api.ReadFile(f, 'r')
    if accept_empty_files and not contents:
      continue
    if not license_re.search(contents):
      bad_files.append(f.LocalPath())
  if bad_files:
    return [output_api.PresubmitPromptWarning(
        'License must match:\n%s\n' % license_re.pattern +
        'Found a bad license header in these files:', items=bad_files)]
  return []


### Other checks

def CheckDoNotSubmit(input_api, output_api):
  return (
      CheckDoNotSubmitInDescription(input_api, output_api) +
      CheckDoNotSubmitInFiles(input_api, output_api)
      )


def CheckTreeIsOpen(input_api, output_api,
                    url=None, closed=None, json_url=None):
  """Check whether to allow commit without prompt.

  Supports two styles:
    1. Checks that an url's content doesn't match a regexp that would mean that
       the tree is closed. (old)
    2. Check the json_url to decide whether to allow commit without prompt.
  Args:
    input_api: input related apis.
    output_api: output related apis.
    url: url to use for regex based tree status.
    closed: regex to match for closed status.
    json_url: url to download json style status.
  """
  if not input_api.is_committing:
    return []
  try:
    if json_url:
      connection = input_api.urllib_request.urlopen(json_url)
      status = input_api.json.loads(connection.read())
      connection.close()
      if not status['can_commit_freely']:
        short_text = 'Tree state is: ' + status['general_state']
        long_text = status['message'] + '\n' + json_url
        return [output_api.PresubmitError(short_text, long_text=long_text)]
    else:
      # TODO(bradnelson): drop this once all users are gone.
      connection = input_api.urllib_request.urlopen(url)
      status = connection.read()
      connection.close()
      if input_api.re.match(closed, status):
        long_text = status + '\n' + url
        return [output_api.PresubmitError('The tree is closed.',
                                          long_text=long_text)]
  except IOError as e:
    return [output_api.PresubmitError('Error fetching tree status.',
                                      long_text=str(e))]
  return []

def GetUnitTestsInDirectory(input_api,
                            output_api,
                            directory,
                            files_to_check=None,
                            files_to_skip=None,
                            env=None,
                            run_on_python2=True,
                            run_on_python3=True,
                            allowlist=None,
                            blocklist=None):
  """Lists all files in a directory and runs them. Doesn't recurse.

  It's mainly a wrapper for RunUnitTests. Use allowlist and blocklist to filter
  tests accordingly.
  """
  unit_tests = []
  test_path = input_api.os_path.abspath(
      input_api.os_path.join(input_api.PresubmitLocalPath(), directory))

  def check(filename, filters):
    return any(True for i in filters if input_api.re.match(i, filename))

  to_run = found = 0
  for filename in input_api.os_listdir(test_path):
    found += 1
    fullpath = input_api.os_path.join(test_path, filename)
    if not input_api.os_path.isfile(fullpath):
      continue
    if files_to_check and not check(filename, files_to_check):
      continue
    if files_to_skip and check(filename, files_to_skip):
      continue
    unit_tests.append(input_api.os_path.join(directory, filename))
    to_run += 1
  input_api.logging.debug('Found %d files, running %d unit tests'
                          % (found, to_run))
  if not to_run:
    return [
        output_api.PresubmitPromptWarning(
          'Out of %d files, found none that matched c=%r, s=%r in directory %s'
          % (found, files_to_check, files_to_skip, directory))
    ]
  return GetUnitTests(
      input_api, output_api, unit_tests, env, run_on_python2, run_on_python3)


def GetUnitTests(
    input_api, output_api, unit_tests, env=None, run_on_python2=True,
    run_on_python3=True):
  """Runs all unit tests in a directory.

  On Windows, sys.executable is used for unit tests ending with ".py".
  """
  assert run_on_python3 or run_on_python2, (
      'At least one of "run_on_python2" or "run_on_python3" must be set.')
  def has_py3_shebang(test):
    with open(test) as f:
      maybe_shebang = f.readline()
    return maybe_shebang.startswith('#!') and 'python3' in maybe_shebang

  # We don't want to hinder users from uploading incomplete patches.
  if input_api.is_committing:
    message_type = output_api.PresubmitError
  else:
    message_type = output_api.PresubmitPromptWarning

  results = []
  for unit_test in unit_tests:
    cmd = [unit_test]
    if input_api.verbose:
      cmd.append('--verbose')
    kwargs = {'cwd': input_api.PresubmitLocalPath()}
    if env:
      kwargs['env'] = env
    if not unit_test.endswith('.py'):
      results.append(input_api.Command(
          name=unit_test,
          cmd=cmd,
          kwargs=kwargs,
          message=message_type))
    else:
      if has_py3_shebang(unit_test) and run_on_python3:
        results.append(input_api.Command(
            name=unit_test,
            cmd=cmd,
            kwargs=kwargs,
            message=message_type,
            python3=True))
      if run_on_python2:
        results.append(input_api.Command(
            name=unit_test,
            cmd=cmd,
            kwargs=kwargs,
            message=message_type))
  return results


def GetUnitTestsRecursively(input_api,
                            output_api,
                            directory,
                            files_to_check,
                            files_to_skip,
                            run_on_python2=True,
                            run_on_python3=True):
  """Gets all files in the directory tree (git repo) that match files_to_check.

  Restricts itself to only find files within the Change's source repo, not
  dependencies.
  """
  def check(filename):
    return (any(input_api.re.match(f, filename) for f in files_to_check) and
            not any(input_api.re.match(f, filename) for f in files_to_skip))

  tests = []

  to_run = found = 0
  for filepath in input_api.change.AllFiles(directory):
    found += 1
    if check(filepath):
      to_run += 1
      tests.append(filepath)
  input_api.logging.debug('Found %d files, running %d' % (found, to_run))
  if not to_run:
    return [
        output_api.PresubmitPromptWarning(
          'Out of %d files, found none that matched c=%r, s=%r in directory %s'
          % (found, files_to_check, files_to_skip, directory))
    ]

  return GetUnitTests(input_api, output_api, tests,
                      run_on_python2=run_on_python2,
                      run_on_python3=run_on_python3)


def GetPythonUnitTests(input_api, output_api, unit_tests):
  """Run the unit tests out of process, capture the output and use the result
  code to determine success.

  DEPRECATED.
  """
  # We don't want to hinder users from uploading incomplete patches.
  if input_api.is_committing:
    message_type = output_api.PresubmitError
  else:
    message_type = output_api.PresubmitNotifyResult
  results = []
  for unit_test in unit_tests:
    # Run the unit tests out of process. This is because some unit tests
    # stub out base libraries and don't clean up their mess. It's too easy to
    # get subtle bugs.
    cwd = None
    env = None
    unit_test_name = unit_test
    # 'python -m test.unit_test' doesn't work. We need to change to the right
    # directory instead.
    if '.' in unit_test:
      # Tests imported in submodules (subdirectories) assume that the current
      # directory is in the PYTHONPATH. Manually fix that.
      unit_test = unit_test.replace('.', '/')
      cwd = input_api.os_path.dirname(unit_test)
      unit_test = input_api.os_path.basename(unit_test)
      env = input_api.environ.copy()
      # At least on Windows, it seems '.' must explicitly be in PYTHONPATH
      backpath = [
          '.', input_api.os_path.pathsep.join(['..'] * (cwd.count('/') + 1))
        ]
      # We convert to str, since on Windows on Python 2 only strings are allowed
      # as environment variables, but literals are unicode since we're importing
      # unicode_literals from __future__.
      if env.get('PYTHONPATH'):
        backpath.append(env.get('PYTHONPATH'))
      env['PYTHONPATH'] = input_api.os_path.pathsep.join((backpath))
      env.pop('VPYTHON_CLEAR_PYTHONPATH', None)
    cmd = [input_api.python_executable, '-m', '%s' % unit_test]
    results.append(input_api.Command(
        name=unit_test_name,
        cmd=cmd,
        kwargs={'env': env, 'cwd': cwd},
        message=message_type))
  return results


def RunUnitTestsInDirectory(input_api, *args, **kwargs):
  """Run tests in a directory serially.

  For better performance, use GetUnitTestsInDirectory and then
  pass to input_api.RunTests.
  """
  return input_api.RunTests(
      GetUnitTestsInDirectory(input_api, *args, **kwargs), False)


def RunUnitTests(input_api, *args, **kwargs):
  """Run tests serially.

  For better performance, use GetUnitTests and then pass to
  input_api.RunTests.
  """
  return input_api.RunTests(GetUnitTests(input_api, *args, **kwargs), False)


def RunPythonUnitTests(input_api, *args, **kwargs):
  """Run python tests in a directory serially.

  DEPRECATED
  """
  return input_api.RunTests(
      GetPythonUnitTests(input_api, *args, **kwargs), False)


def _FetchAllFiles(input_api, files_to_check, files_to_skip):
  """Hack to fetch all files."""
  # We cannot use AffectedFiles here because we want to test every python
  # file on each single python change. It's because a change in a python file
  # can break another unmodified file.
  # Use code similar to InputApi.FilterSourceFile()
  def Find(filepath, filters):
    if input_api.platform == 'win32':
      filepath = filepath.replace('\\', '/')

    for item in filters:
      if input_api.re.match(item, filepath):
        return True
    return False

  files = []
  path_len = len(input_api.PresubmitLocalPath())
  for dirpath, dirnames, filenames in input_api.os_walk(
      input_api.PresubmitLocalPath()):
    # Passes dirnames in block list to speed up search.
    for item in dirnames[:]:
      filepath = input_api.os_path.join(dirpath, item)[path_len + 1:]
      if Find(filepath, files_to_skip):
        dirnames.remove(item)
    for item in filenames:
      filepath = input_api.os_path.join(dirpath, item)[path_len + 1:]
      if Find(filepath, files_to_check) and not Find(filepath, files_to_skip):
        files.append(filepath)
  return files


def GetPylint(input_api,
              output_api,
              files_to_check=None,
              files_to_skip=None,
              disabled_warnings=None,
              extra_paths_list=None,
              pylintrc=None):
  """Run pylint on python files.

  The default files_to_check enforces looking only at *.py files.
  """

  files_to_check = tuple(files_to_check or (r'.*\.py$', ))
  files_to_skip = tuple(files_to_skip or input_api.DEFAULT_FILES_TO_SKIP)
  extra_paths_list = extra_paths_list or []

  if input_api.is_committing:
    error_type = output_api.PresubmitError
  else:
    error_type = output_api.PresubmitPromptWarning

  # Only trigger if there is at least one python file affected.
  def rel_path(regex):
    """Modifies a regex for a subject to accept paths relative to root."""
    def samefile(a, b):
      # Default implementation for platforms lacking os.path.samefile
      # (like Windows).
      return input_api.os_path.abspath(a) == input_api.os_path.abspath(b)
    samefile = getattr(input_api.os_path, 'samefile', samefile)
    if samefile(input_api.PresubmitLocalPath(),
                input_api.change.RepositoryRoot()):
      return regex

    prefix = input_api.os_path.join(input_api.os_path.relpath(
        input_api.PresubmitLocalPath(), input_api.change.RepositoryRoot()), '')
    return input_api.re.escape(prefix) + regex
  src_filter = lambda x: input_api.FilterSourceFile(
      x, map(rel_path, files_to_check), map(rel_path, files_to_skip))
  if not input_api.AffectedSourceFiles(src_filter):
    input_api.logging.info('Skipping pylint: no matching changes.')
    return []

  if pylintrc is not None:
    pylintrc = input_api.os_path.join(input_api.PresubmitLocalPath(), pylintrc)
  else:
    pylintrc = input_api.os_path.join(_HERE, 'pylintrc')
  extra_args = ['--rcfile=%s' % pylintrc]
  if disabled_warnings:
    extra_args.extend(['-d', ','.join(disabled_warnings)])

  files = _FetchAllFiles(input_api, files_to_check, files_to_skip)
  if not files:
    return []
  files.sort()

  input_api.logging.info('Running pylint on %d files', len(files))
  input_api.logging.debug('Running pylint on: %s', files)
  env = input_api.environ.copy()
  env['PYTHONPATH'] = input_api.os_path.pathsep.join(extra_paths_list)
  env.pop('VPYTHON_CLEAR_PYTHONPATH', None)
  input_api.logging.debug('  with extra PYTHONPATH: %r', extra_paths_list)

  def GetPylintCmd(flist, extra, parallel):
    # Windows needs help running python files so we explicitly specify
    # the interpreter to use. It also has limitations on the size of
    # the command-line, so we pass arguments via a pipe.
    tool = input_api.os_path.join(_HERE, 'pylint')
    kwargs = {'env': env}
    if input_api.platform == 'win32':
      # On Windows, scripts on the current directory take precedence over PATH.
      # When `pylint.bat` calls `vpython`, it will execute the `vpython` of the
      # depot_tools under test instead of the one in the bot.
      # As a workaround, we run the tests from the parent directory instead.
      cwd = input_api.change.RepositoryRoot()
      if input_api.os_path.basename(cwd) == 'depot_tools':
        kwargs['cwd'] = input_api.os_path.dirname(cwd)
        flist = [input_api.os_path.join('depot_tools', f) for f in flist]
      tool += '.bat'

    cmd = [tool, '--args-on-stdin']
    if len(flist) == 1:
      description = flist[0]
    else:
      description = '%s files' % len(flist)

    args = extra_args[:]
    if extra:
      args.extend(extra)
      description += ' using %s' % (extra,)
    if parallel:
      args.append('--jobs=%s' % input_api.cpu_count)
      description += ' on %d cores' % input_api.cpu_count

    kwargs['stdin'] = '\n'.join(args + flist)

    return input_api.Command(
        name='Pylint (%s)' % description,
        cmd=cmd,
        kwargs=kwargs,
        message=error_type)

  # Always run pylint and pass it all the py files at once.
  # Passing py files one at time is slower and can produce
  # different results.  input_api.verbose used to be used
  # to enable this behaviour but differing behaviour in
  # verbose mode is not desirable.
  # Leave this unreachable code in here so users can make
  # a quick local edit to diagnose pylint issues more
  # easily.
  if True:
    # pylint's cycle detection doesn't work in parallel, so spawn a second,
    # single-threaded job for just that check.

    # Some PRESUBMITs explicitly mention cycle detection.
    if not any('R0401' in a or 'cyclic-import' in a for a in extra_args):
      return [
        GetPylintCmd(files, ["--disable=cyclic-import"], True),
        GetPylintCmd(files, ["--disable=all", "--enable=cyclic-import"], False)
      ]
    else:
      return [ GetPylintCmd(files, [], True) ]

  else:
    return map(lambda x: GetPylintCmd([x], [], 1), files)


def RunPylint(input_api, *args, **kwargs):
  """Legacy presubmit function.

  For better performance, get all tests and then pass to
  input_api.RunTests.
  """
  return input_api.RunTests(GetPylint(input_api, *args, **kwargs), False)


def CheckDirMetadataFormat(input_api, output_api, dirmd_bin=None):
  # TODO(crbug.com/1102997): Remove OWNERS once DIR_METADATA migration is
  # complete.
  file_filter = lambda f: (
      input_api.basename(f.LocalPath()) in ('DIR_METADATA', 'OWNERS'))
  affected_files = set([
      f.AbsoluteLocalPath()
      for f in input_api.change.AffectedFiles(
          include_deletes=False, file_filter=file_filter)
  ])
  if not affected_files:
    return []

  name = 'Validate metadata in OWNERS and DIR_METADATA files'
  kwargs = {}

  if dirmd_bin is None:
    dirmd_bin = 'dirmd.bat' if input_api.is_windows else 'dirmd'
  cmd = [dirmd_bin, 'validate'] + sorted(affected_files)

  return [input_api.Command(
      name, cmd, kwargs, output_api.PresubmitError)]


def CheckNoNewMetadataInOwners(input_api, output_api):
  """Check that no metadata is added to OWNERS files."""
  _METADATA_LINE_RE = input_api.re.compile(
      r'^#\s*(TEAM|COMPONENT|OS|WPT-NOTIFY)+\s*:\s*\S+$',
      input_api.re.MULTILINE | input_api.re.IGNORECASE)
  affected_files = input_api.change.AffectedFiles(
      include_deletes=False,
      file_filter=lambda f: input_api.basename(f.LocalPath()) == 'OWNERS')

  errors = []
  for f in affected_files:
    for _, line in f.ChangedContents():
      if _METADATA_LINE_RE.search(line):
        errors.append(f.AbsoluteLocalPath())
        break

  if not errors:
    return []

  return [output_api.PresubmitError(
      'New metadata was added to the following OWNERS files, but should '
      'have been added to DIR_METADATA files instead:\n' +
      '\n'.join(errors) + '\n' +
      'See https://source.chromium.org/chromium/infra/infra/+/HEAD:'
      'go/src/infra/tools/dirmd/proto/dir_metadata.proto for details.')]


def CheckOwnersDirMetadataExclusive(input_api, output_api):
  """Check that metadata in OWNERS files and DIR_METADATA files are mutually
  exclusive.
  """
  _METADATA_LINE_RE = input_api.re.compile(
      r'^#\s*(TEAM|COMPONENT|OS|WPT-NOTIFY)+\s*:\s*\S+$',
      input_api.re.MULTILINE)
  file_filter = (
      lambda f: input_api.basename(f.LocalPath()) in ('OWNERS', 'DIR_METADATA'))
  affected_dirs = set([
      input_api.os_path.dirname(f.AbsoluteLocalPath())
      for f in input_api.change.AffectedFiles(
          include_deletes=False, file_filter=file_filter)
  ])

  errors = []
  for path in affected_dirs:
    owners_path = input_api.os_path.join(path, 'OWNERS')
    dir_metadata_path = input_api.os_path.join(path, 'DIR_METADATA')
    if (not input_api.os_path.isfile(dir_metadata_path)
        or not input_api.os_path.isfile(owners_path)):
      continue
    if _METADATA_LINE_RE.search(input_api.ReadFile(owners_path)):
      errors.append(owners_path)

  if not errors:
    return []

  return [output_api.PresubmitError(
      'The following OWNERS files should contain no metadata, as there is a '
      'DIR_METADATA file present in the same directory:\n'
      + '\n'.join(errors))]


def CheckOwnersFormat(input_api, output_api):
  if input_api.gerrit and input_api.gerrit.IsCodeOwnersEnabledOnRepo():
    return []
  affected_files = set([
      f.LocalPath()
      for f in input_api.change.AffectedFiles()
      if 'OWNERS' in f.LocalPath() and f.Action() != 'D'
  ])
  if not affected_files:
    return []
  try:
    owners_db = input_api.owners_db
    owners_db.override_files = {}
    owners_db.load_data_needed_for(affected_files)
    return []
  except Exception as e:
    return [output_api.PresubmitError(
        'Error parsing OWNERS files:\n%s' % e)]


def CheckOwners(
    input_api, output_api, source_file_filter=None, allow_tbr=True):
  # Skip OWNERS check when Owners-Override label is approved. This is intended
  # for global owners, trusted bots, and on-call sheriffs. Review is still
  # required for these changes.
  if (input_api.change.issue
      and input_api.gerrit.IsOwnersOverrideApproved(input_api.change.issue)):
    return []

  if input_api.gerrit and input_api.gerrit.IsCodeOwnersEnabledOnRepo():
    return []

  affected_files = set([f.LocalPath() for f in
      input_api.change.AffectedFiles(file_filter=source_file_filter)])
  owner_email, reviewers = GetCodereviewOwnerAndReviewers(
      input_api, approval_needed=input_api.is_committing)

  owner_email = owner_email or input_api.change.author_email

  approval_status = input_api.owners_client.GetFilesApprovalStatus(
      affected_files, reviewers.union([owner_email]), [])
  missing_files = [
      f for f in affected_files
      if approval_status[f] != input_api.owners_client.APPROVED]
  affects_owners = any('OWNERS' in name for name in missing_files)

  if input_api.is_committing:
    if input_api.tbr and not affects_owners:
      return [output_api.PresubmitNotifyResult(
          '--tbr was specified, skipping OWNERS check')]
    needed = 'LGTM from an OWNER'
    output_fn = output_api.PresubmitError
    if input_api.change.issue:
      if input_api.dry_run:
        output_fn = lambda text: output_api.PresubmitNotifyResult(
            'This is a dry run, but these failures would be reported on ' +
            'commit:\n' + text)
    else:
      return [output_api.PresubmitError(
          'OWNERS check failed: this CL has no Gerrit change number, '
          'so we can\'t check it for approvals.')]
  else:
    needed = 'OWNER reviewers'
    output_fn = output_api.PresubmitNotifyResult

  if missing_files:
    output_list = [
        output_fn('Missing %s for these files:\n    %s' %
                  (needed, '\n    '.join(sorted(missing_files))))]
    if input_api.tbr and affects_owners:
      output_list.append(output_fn('TBR for OWNERS files are ignored.'))
    if not input_api.is_committing:
      suggested_owners = input_api.owners_client.SuggestOwners(
          missing_files, exclude=[owner_email])
      output_list.append(output_fn('Suggested OWNERS: ' +
          '(Use "git-cl owners" to interactively select owners.)\n    %s' %
          ('\n    '.join(suggested_owners))))
    return output_list

  if (input_api.is_committing and not reviewers and
      not input_api.gerrit.IsBotCommitApproved(input_api.change.issue)):
    return [output_fn('Missing LGTM from someone other than %s' % owner_email)]
  return []


def GetCodereviewOwnerAndReviewers(
    input_api, _email_regexp=None, approval_needed=True):
  """Return the owner and reviewers of a change, if any.

  If approval_needed is True, only reviewers who have approved the change
  will be returned.
  """
  # Recognizes 'X@Y' email addresses. Very simplistic.
  EMAIL_REGEXP = input_api.re.compile(r'^[\w\-\+\%\.]+\@[\w\-\+\%\.]+$')
  issue = input_api.change.issue
  if not issue:
    return None, (set() if approval_needed else
                  _ReviewersFromChange(input_api.change))

  owner_email = input_api.gerrit.GetChangeOwner(issue)
  reviewers = set(
      r for r in input_api.gerrit.GetChangeReviewers(issue, approval_needed)
      if _match_reviewer_email(r, owner_email, EMAIL_REGEXP))
  input_api.logging.debug('owner: %s; approvals given by: %s',
                          owner_email, ', '.join(sorted(reviewers)))
  return owner_email, reviewers


def _ReviewersFromChange(change):
  """Return the reviewers specified in the |change|, if any."""
  reviewers = set()
  reviewers.update(change.ReviewersFromDescription())
  reviewers.update(change.TBRsFromDescription())

  # Drop reviewers that aren't specified in email address format.
  return set(reviewer for reviewer in reviewers if '@' in reviewer)


def _match_reviewer_email(r, owner_email, email_regexp):
  return email_regexp.match(r) and r != owner_email


def CheckSingletonInHeaders(input_api, output_api, source_file_filter=None):
  """Deprecated, must be removed."""
  return [
    output_api.PresubmitNotifyResult(
        'CheckSingletonInHeaders is deprecated, please remove it.')
  ]


def PanProjectChecks(input_api, output_api,
                     excluded_paths=None, text_files=None,
                     license_header=None, project_name=None,
                     owners_check=True, maxlen=80):
  """Checks that ALL chromium orbit projects should use.

  These are checks to be run on all Chromium orbit project, including:
    Chromium
    Native Client
    V8
  When you update this function, please take this broad scope into account.
  Args:
    input_api: Bag of input related interfaces.
    output_api: Bag of output related interfaces.
    excluded_paths: Don't include these paths in common checks.
    text_files: Which file are to be treated as documentation text files.
    license_header: What license header should be on files.
    project_name: What is the name of the project as it appears in the license.
  Returns:
    A list of warning or error objects.
  """
  excluded_paths = tuple(excluded_paths or [])
  text_files = tuple(text_files or (
      r'.+\.txt$',
      r'.+\.json$',
  ))

  results = []
  # This code loads the default skip list (e.g. third_party, experimental, etc)
  # and add our skip list (breakpad, skia and v8 are still not following
  # google style and are not really living this repository).
  # See presubmit_support.py InputApi.FilterSourceFile for the (simple) usage.
  files_to_skip = input_api.DEFAULT_FILES_TO_SKIP + excluded_paths
  files_to_check = input_api.DEFAULT_FILES_TO_CHECK + text_files
  sources = lambda x: input_api.FilterSourceFile(x, files_to_skip=files_to_skip)
  text_files = lambda x: input_api.FilterSourceFile(
      x, files_to_skip=files_to_skip, files_to_check=files_to_check)

  snapshot_memory = []
  def snapshot(msg):
    """Measures & prints performance warning if a rule is running slow."""
    if input_api.sys.version_info.major == 2:
      dt2 = input_api.time.clock()
    else:
      dt2 = input_api.time.process_time()
    if snapshot_memory:
      delta_ms = int(1000*(dt2 - snapshot_memory[0]))
      if delta_ms > 500:
        print("  %s took a long time: %dms" % (snapshot_memory[1], delta_ms))
    snapshot_memory[:] = (dt2, msg)

  snapshot("checking owners files format")
  results.extend(input_api.canned_checks.CheckOwnersFormat(
      input_api, output_api))

  if owners_check:
    snapshot("checking owners")
    results.extend(input_api.canned_checks.CheckOwners(
        input_api, output_api, source_file_filter=None))

  snapshot("checking long lines")
  results.extend(input_api.canned_checks.CheckLongLines(
      input_api, output_api, maxlen, source_file_filter=sources))
  snapshot( "checking tabs")
  results.extend(input_api.canned_checks.CheckChangeHasNoTabs(
      input_api, output_api, source_file_filter=sources))
  snapshot( "checking stray whitespace")
  results.extend(input_api.canned_checks.CheckChangeHasNoStrayWhitespace(
      input_api, output_api, source_file_filter=sources))
  snapshot("checking license")
  results.extend(input_api.canned_checks.CheckLicense(
      input_api, output_api, license_header, project_name,
      source_file_filter=sources))

  if input_api.is_committing:
    snapshot("checking was uploaded")
    results.extend(input_api.canned_checks.CheckChangeWasUploaded(
        input_api, output_api))
    snapshot("checking description")
    results.extend(input_api.canned_checks.CheckChangeHasDescription(
        input_api, output_api))
    results.extend(input_api.canned_checks.CheckDoNotSubmitInDescription(
        input_api, output_api))
    snapshot("checking do not submit in files")
    results.extend(input_api.canned_checks.CheckDoNotSubmitInFiles(
        input_api, output_api))
    if input_api.change.scm == 'git':
      snapshot("checking for commit objects in tree")
      results.extend(input_api.canned_checks.CheckForCommitObjects(
          input_api, output_api))
  snapshot("done")
  return results


def CheckPatchFormatted(input_api,
                        output_api,
                        bypass_warnings=True,
                        check_clang_format=True,
                        check_js=False,
                        check_python=None,
                        result_factory=None):
  result_factory = result_factory or output_api.PresubmitPromptWarning
  import git_cl

  display_args = []
  if not check_clang_format:
    display_args.append('--no-clang-format')

  if check_js:
    display_args.append('--js')

  # Explicitly setting check_python to will enable/disable python formatting
  # on all files. Leaving it as None will enable checking patch formatting
  # on files that have a .style.yapf file in a parent directory.
  if check_python is not None:
    if check_python:
      display_args.append('--python')
    else:
      display_args.append('--no-python')

  cmd = ['-C', input_api.change.RepositoryRoot(),
         'cl', 'format', '--dry-run', '--presubmit'] + display_args
  presubmit_subdir = input_api.os_path.relpath(
      input_api.PresubmitLocalPath(), input_api.change.RepositoryRoot())
  if presubmit_subdir.startswith('..') or presubmit_subdir == '.':
    presubmit_subdir = ''
  # If the PRESUBMIT.py is in a parent repository, then format the entire
  # subrepository. Otherwise, format only the code in the directory that
  # contains the PRESUBMIT.py.
  if presubmit_subdir:
    cmd.append(input_api.PresubmitLocalPath())
  code, _ = git_cl.RunGitWithCode(cmd, suppress_stderr=bypass_warnings)
  # bypass_warnings? Only fail with code 2.
  # As this is just a warning, ignore all other errors if the user
  # happens to have a broken clang-format, doesn't use git, etc etc.
  if code == 2 or (code and not bypass_warnings):
    if presubmit_subdir:
      short_path = presubmit_subdir
    else:
      short_path = input_api.basename(input_api.change.RepositoryRoot())
    display_args.append(presubmit_subdir)
    return [result_factory(
      'The %s directory requires source formatting. '
      'Please run: git cl format %s' %
      (short_path, ' '.join(display_args)))]
  return []


def CheckGNFormatted(input_api, output_api):
  import gn
  affected_files = input_api.AffectedFiles(
      include_deletes=False,
      file_filter=lambda x: x.LocalPath().endswith('.gn') or
                            x.LocalPath().endswith('.gni') or
                            x.LocalPath().endswith('.typemap'))
  warnings = []
  for f in affected_files:
    cmd = ['gn', 'format', '--dry-run', f.AbsoluteLocalPath()]
    rc = gn.main(cmd)
    if rc == 2:
      warnings.append(output_api.PresubmitPromptWarning(
          '%s requires formatting. Please run:\n  gn format %s' % (
              f.AbsoluteLocalPath(), f.LocalPath())))
  # It's just a warning, so ignore other types of failures assuming they'll be
  # caught elsewhere.
  return warnings


def CheckCIPDManifest(input_api, output_api, path=None, content=None):
  """Verifies that a CIPD ensure file manifest is valid against all platforms.

  Exactly one of "path" or "content" must be provided. An assertion will occur
  if neither or both are provided.

  Args:
    path (str): If provided, the filesystem path to the manifest to verify.
    content (str): If provided, the raw content of the manifest to veirfy.
  """
  cipd_bin = 'cipd' if not input_api.is_windows else 'cipd.bat'
  cmd = [cipd_bin, 'ensure-file-verify']
  kwargs = {}

  if input_api.is_windows:
    # Needs to be able to resolve "cipd.bat".
    kwargs['shell'] = True

  if input_api.verbose:
    cmd += ['-log-level', 'debug']

  if path:
    assert content is None, 'Cannot provide both "path" and "content".'
    cmd += ['-ensure-file', path]
    name = 'Check CIPD manifest %r' % path
  elif content:
    assert path is None, 'Cannot provide both "path" and "content".'
    cmd += ['-ensure-file=-']
    kwargs['stdin'] = content
    # quick and dirty parser to extract checked packages.
    packages = [
      l.split()[0] for l in (ll.strip() for ll in content.splitlines())
      if ' ' in l and not l.startswith('$')
    ]
    name = 'Check CIPD packages from string: %r' % (packages,)
  else:
    raise Exception('Exactly one of "path" or "content" must be provided.')

  return input_api.Command(
      name,
      cmd,
      kwargs,
      output_api.PresubmitError)


def CheckCIPDPackages(input_api, output_api, platforms, packages):
  """Verifies that all named CIPD packages can be resolved against all supplied
  platforms.

  Args:
    platforms (list): List of CIPD platforms to verify.
    packages (dict): Mapping of package name to version.
  """
  manifest = []
  for p in platforms:
    manifest.append('$VerifiedPlatform %s' % (p,))
  for k, v in packages.items():
    manifest.append('%s %s' % (k, v))
  return CheckCIPDManifest(input_api, output_api, content='\n'.join(manifest))


def CheckCIPDClientDigests(input_api, output_api, client_version_file):
  """Verifies that *.digests file was correctly regenerated.

  <client_version_file>.digests file contains pinned hashes of the CIPD client.
  It is consulted during CIPD client bootstrap and self-update. It should be
  regenerated each time CIPD client version file changes.

  Args:
    client_version_file (str): Path to a text file with CIPD client version.
  """
  cmd = [
    'cipd' if not input_api.is_windows else 'cipd.bat',
    'selfupdate-roll', '-check', '-version-file', client_version_file,
  ]
  if input_api.verbose:
    cmd += ['-log-level', 'debug']
  return input_api.Command(
      'Check CIPD client_version_file.digests file',
      cmd,
      {'shell': True} if input_api.is_windows else {},  # to resolve cipd.bat
      output_api.PresubmitError)


def CheckForCommitObjects(input_api, output_api):
  """Validates that there are no commit objects in the repository.

  Commit objects are put into the git tree typically by submodule tooling.
  Because we use gclient to handle external repository references instead,
  we want to avoid this. Having commit objects in the tree can confuse git
  tooling in some scenarios into thinking that the tree is dirty (e.g. the
  presence of a DEPS subrepo at a location where a commit object is stored
  in the tree).

  Args:
    input_api: Bag of input related interfaces.
    output_api: Bag of output related interfaces.

  Returns:
    A presubmit error if a commit object is present in the tree.
  """

  def parse_tree_entry(ent):
    """Splits a tree entry into components

    Args:
      ent: a tree entry in the form "filemode type hash\tname"

    Returns:
      The tree entry split into component parts
    """
    tabparts = ent.split('\t', 1)
    spaceparts = tabparts[0].split(' ', 2)
    return (spaceparts[0], spaceparts[1], spaceparts[2], tabparts[1])

  full_tree = input_api.subprocess.check_output(
          ['git', 'ls-tree', '-r', '--full-tree', 'HEAD'],
          cwd=input_api.PresubmitLocalPath()
        ).decode('utf8')
  tree_entries = full_tree.split('\n')
  tree_entries = [x for x in tree_entries if len(x) > 0]
  tree_entries = map(parse_tree_entry, tree_entries)
  bad_tree_entries = [x for x in tree_entries if x[1] == 'commit']
  bad_tree_entries = [x[3] for x in bad_tree_entries]
  if len(bad_tree_entries) > 0:
    return [output_api.PresubmitError(
      'Commit objects present within tree.\n'
      'This may be due to submodule-related interactions; the presence of a\n'
      'commit object in the tree may lead to odd situations where files are\n'
      'inconsistently checked-out. Remove these commit entries and validate\n'
      'your changeset again:\n',
      bad_tree_entries)]
  return []


def CheckVPythonSpec(input_api, output_api, file_filter=None):
  """Validates any changed .vpython files with vpython verification tool.

  Args:
    input_api: Bag of input related interfaces.
    output_api: Bag of output related interfaces.
    file_filter: Custom function that takes a path (relative to client root) and
      returns boolean, which is used to filter files for which to apply the
      verification to. Defaults to any path ending with .vpython, which captures
      both global .vpython and <script>.vpython files.

  Returns:
    A list of input_api.Command objects containing verification commands.
  """
  file_filter = file_filter or (lambda f: f.LocalPath().endswith('.vpython'))
  affected_files = input_api.AffectedTestableFiles(file_filter=file_filter)
  affected_files = map(lambda f: f.AbsoluteLocalPath(), affected_files)

  commands = []
  for f in affected_files:
    commands.append(input_api.Command(
      'Verify %s' % f,
      ['vpython', '-vpython-spec', f, '-vpython-tool', 'verify'],
      {'stderr': input_api.subprocess.STDOUT},
      output_api.PresubmitError))

  return commands


def CheckChangedLUCIConfigs(input_api, output_api):
  import collections
  import base64
  import json
  import logging

  import auth
  import git_cl

  LUCI_CONFIG_HOST_NAME = 'luci-config.appspot.com'

  cl = git_cl.Changelist()
  if input_api.change.issue and input_api.gerrit:
    remote_branch = input_api.gerrit.GetDestRef(input_api.change.issue)
  else:
    remote, remote_branch = cl.GetRemoteBranch()
    if remote_branch.startswith('refs/remotes/%s/' % remote):
      remote_branch = remote_branch.replace(
          'refs/remotes/%s/' % remote, 'refs/heads/', 1)
    if remote_branch.startswith('refs/remotes/branch-heads/'):
      remote_branch = remote_branch.replace(
          'refs/remotes/branch-heads/', 'refs/branch-heads/', 1)

  remote_host_url = cl.GetRemoteUrl()
  if not remote_host_url:
    return [output_api.PresubmitError(
        'Remote host url for git has not been defined')]
  remote_host_url = remote_host_url.rstrip('/')
  if remote_host_url.endswith('.git'):
    remote_host_url = remote_host_url[:-len('.git')]

  # authentication
  try:
    acc_tkn = auth.Authenticator().get_access_token()
  except auth.LoginRequiredError as e:
    return [output_api.PresubmitError(
        'Error in authenticating user.', long_text=str(e))]

  def request(endpoint, body=None):
    api_url = ('https://%s/_ah/api/config/v1/%s'
               % (LUCI_CONFIG_HOST_NAME, endpoint))
    req = input_api.urllib_request.Request(api_url)
    req.add_header('Authorization', 'Bearer %s' % acc_tkn.token)
    if body is not None:
      req.add_header('Content-Type', 'application/json')
      req.data = json.dumps(body).encode('utf-8')
    return json.load(input_api.urllib_request.urlopen(req))

  try:
    config_sets = request('config-sets').get('config_sets')
  except input_api.urllib_error.HTTPError as e:
    return [output_api.PresubmitError(
        'Config set request to luci-config failed', long_text=str(e))]
  if not config_sets:
    return [output_api.PresubmitPromptWarning('No config_sets were returned')]
  loc_pref = '%s/+/%s/' % (remote_host_url, remote_branch)
  logging.debug('Derived location prefix: %s', loc_pref)
  dir_to_config_set = {
    '%s/' % cs['location'][len(loc_pref):].rstrip('/'): cs['config_set']
    for cs in config_sets
    if cs['location'].startswith(loc_pref) or
    ('%s/' % cs['location']) == loc_pref
  }
  if not dir_to_config_set:
    warning_long_text_lines = [
        'No config_set found for %s.' % loc_pref,
        'Found the following:',
    ]
    for loc in sorted(cs['location'] for cs in config_sets):
      warning_long_text_lines.append('  %s' % loc)
    warning_long_text_lines.append('')
    warning_long_text_lines.append(
        'If the requested location is internal,'
        ' the requester may not have access.')

    return [output_api.PresubmitPromptWarning(
        warning_long_text_lines[0],
        long_text='\n'.join(warning_long_text_lines))]
  cs_to_files = collections.defaultdict(list)
  for f in input_api.AffectedFiles(include_deletes=False):
    # windows
    file_path = f.LocalPath().replace(_os.sep, '/')
    logging.debug('Affected file path: %s', file_path)
    for dr, cs in dir_to_config_set.items():
      if dr == '/' or file_path.startswith(dr):
        cs_to_files[cs].append({
          'path': file_path[len(dr):] if dr != '/' else file_path,
          'content': base64.b64encode(
              '\n'.join(f.NewContents()).encode('utf-8')).decode('utf-8')
        })
  outputs = []
  for cs, f in cs_to_files.items():
    try:
      # TODO(myjang): parallelize
      res = request(
          'validate-config', body={'config_set': cs, 'files': f})
    except input_api.urllib_error.HTTPError as e:
      return [output_api.PresubmitError(
          'Validation request to luci-config failed', long_text=str(e))]
    for msg in res.get('messages', []):
      sev = msg['severity']
      if sev == 'WARNING':
        out_f = output_api.PresubmitPromptWarning
      elif sev == 'ERROR' or sev == 'CRITICAL':
        out_f = output_api.PresubmitError
      else:
        out_f = output_api.PresubmitNotifyResult
      outputs.append(out_f('Config validation: %s' % msg['text']))
  return outputs


def CheckLucicfgGenOutput(input_api, output_api, entry_script):
  """Verifies configs produced by `lucicfg` are up-to-date and pass validation.

  Runs the check unconditionally, regardless of what files are modified. Examine
  input_api.AffectedFiles() yourself before using CheckLucicfgGenOutput if this
  is a concern.

  Assumes `lucicfg` binary is in PATH and the user is logged in.

  Args:
    entry_script: path to the entry-point *.star script responsible for
        generating a single config set. Either absolute or relative to the
        currently running PRESUBMIT.py script.

  Returns:
    A list of input_api.Command objects containing verification commands.
  """
  return [
      input_api.Command(
        'lucicfg validate "%s"' % entry_script,
        [
            'lucicfg' if not input_api.is_windows else 'lucicfg.bat',
            'validate', entry_script,
            '-log-level', 'debug' if input_api.verbose else 'warning',
        ],
        {
          'stderr': input_api.subprocess.STDOUT,
          'shell': input_api.is_windows,  # to resolve *.bat
          'cwd': input_api.PresubmitLocalPath(),
        },
        output_api.PresubmitError)
  ]

def CheckJsonParses(input_api, output_api):
  """Verifies that all JSON files at least parse as valid JSON."""
  import json
  affected_files = input_api.AffectedFiles(
      include_deletes=False,
      file_filter=lambda x: x.LocalPath().endswith('.json'))
  warnings = []
  for f in affected_files:
    with open(f.AbsoluteLocalPath()) as j:
      try:
        json.load(j)
      except ValueError:
        # Just a warning for now, in case people are using JSON5 somewhere.
        warnings.append(output_api.PresubmitPromptWarning(
            '%s does not appear to be valid JSON.' % f.LocalPath()))
  return warnings

# string pattern, sequence of strings to show when pattern matches,
# error flag. True if match is a presubmit error, otherwise it's a warning.
_NON_INCLUSIVE_TERMS = (
    (
        # Note that \b pattern in python re is pretty particular. In this
        # regexp, 'class WhiteList ...' will match, but 'class FooWhiteList
        # ...' will not. This may require some tweaking to catch these cases
        # without triggering a lot of false positives. Leaving it naive and
        # less matchy for now.
        r'/\b(?i)((black|white)list|slave)\b',  # nocheck
        (
            'Please don\'t use blacklist, whitelist, '  # nocheck
            'or slave in your',  # nocheck
            'code and make every effort to use other terms. Using "// nocheck"',
            '"# nocheck" or "<!-- nocheck -->"',
            'at the end of the offending line will bypass this PRESUBMIT error',
            'but avoid using this whenever possible. Reach out to',
            'community@chromium.org if you have questions'),
        True),)


def _GetMessageForMatchingTerm(input_api, affected_file, line_number, line,
                               term, message):
  """Helper method for CheckInclusiveLanguage.

  Returns an string composed of the name of the file, the line number where the
  match has been found and the additional text passed as |message| in case the
  target type name matches the text inside the line passed as parameter.
  """
  result = []

  # A // nocheck comment will bypass this error.
  if line.endswith(" nocheck") or line.endswith("<!-- nocheck -->"):
    return result

  # Ignore C-style single-line comments about banned terms.
  if input_api.re.search(r"//.*$", line):
    line = input_api.re.sub(r"//.*$", "", line)

  # Ignore lines from C-style multi-line comments.
  if input_api.re.search(r"^\s*\*", line):
    return result

  # Ignore Python-style comments about banned terms.
  # This actually removes comment text from the first # on.
  if input_api.re.search(r"#.*$", line):
    line = input_api.re.sub(r"#.*$", "", line)

  matched = False
  if term[0:1] == '/':
    regex = term[1:]
    if input_api.re.search(regex, line):
      matched = True
  elif term in line:
    matched = True

  if matched:
    result.append('    %s:%d:' % (affected_file.LocalPath(), line_number))
    for message_line in message:
      result.append('      %s' % message_line)

  return result


def CheckInclusiveLanguage(input_api, output_api,
                           excluded_directories_relative_path=None,
                           non_inclusive_terms=_NON_INCLUSIVE_TERMS):
  """Make sure that banned non-inclusive terms are not used."""

  # Presubmit checks may run on a bot where the changes are actually
  # in a repo that isn't chromium/src (e.g., when testing src + tip-of-tree
  # ANGLE), but this particular check only makes sense for changes to
  # chromium/src.
  if input_api.change.RepositoryRoot() != input_api.PresubmitLocalPath():
    return []

  warnings = []
  errors = []

  if excluded_directories_relative_path is None:
    excluded_directories_relative_path = [
        'infra',
        'inclusive_language_presubmit_exempt_dirs.txt'
    ]

  # Note that this matches exact path prefixes, and does not match
  # subdirectories. Only files directly in an exlcluded path will
  # match.
  def IsExcludedFile(affected_file, excluded_paths):
    local_dir = input_api.os_path.dirname(affected_file.LocalPath())

    return local_dir in excluded_paths

  def CheckForMatch(affected_file, line_num, line, term, message, error):
    problems = _GetMessageForMatchingTerm(input_api, affected_file, line_num,
                                          line, term, message)

    if problems:
      if error:
        errors.extend(problems)
      else:
        warnings.extend(problems)

  excluded_paths = []
  dirs_file_path = input_api.os_path.join(input_api.change.RepositoryRoot(),
                                          *excluded_directories_relative_path)
  f = input_api.ReadFile(dirs_file_path)

  for line in f.splitlines():
    path = line.split()[0]
    if len(path) > 0:
      excluded_paths.append(path)

  excluded_paths = set(excluded_paths)
  for f in input_api.AffectedFiles():
    for line_num, line in f.ChangedContents():
      for term, message, error in non_inclusive_terms:
        if IsExcludedFile(f, excluded_paths):
          continue
        CheckForMatch(f, line_num, line, term, message, error)

  result = []
  if (warnings):
    result.append(
        output_api.PresubmitPromptWarning(
            'Banned non-inclusive language was used.\n' + '\n'.join(warnings)))
  if (errors):
    result.append(
        output_api.PresubmitError('Banned non-inclusive language was used.\n' +
                                  '\n'.join(errors)))
  return result
