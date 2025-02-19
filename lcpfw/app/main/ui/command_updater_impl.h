// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COMMAND_UPDATER_IMPL_H_
#define CHROME_BROWSER_COMMAND_UPDATER_IMPL_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "base/macros.h"

#include "main/ui/command_updater.h"

////////////////////////////////////////////////////////////////////////////////
//
// CommandUpdaterImpl class
//
//   This object manages the enabled state of a set of commands. Observers
//   register to listen to changes in this state so they can update their
//   presentation.
//
class CommandUpdaterImpl : public CommandUpdater {
 public:
  // Create a CommandUpdaterImpl with |delegate| to handle the execution of
  // specific commands.
  explicit CommandUpdaterImpl(CommandUpdaterDelegate* delegate);
  ~CommandUpdaterImpl() override;

  // Override from CommandUpdater:
  bool SupportsCommand(int id) const override;
  bool IsCommandEnabled(int id) const override;
  bool ExecuteCommand(
      int id,
      base::TimeTicks time_stamp = base::TimeTicks::Now()) override;
  bool ExecuteCommandWithParams(
      int id,
      const CommandParamsDetails& params,
      base::TimeTicks time_stamp = base::TimeTicks::Now()) override;
  bool UpdateCommandEnabled(int id, bool state) override;

  void DisableAllCommands();
  std::vector<int> GetAllIds();

 private:
  // A piece of data about a command - whether or not it is enabled, and a list
  // of objects that observe the enabled state of this command.
  class Command;

  // Get a Command node for a given command ID, creating an entry if it doesn't
  // exist if desired.
  Command* GetCommand(int id, bool create);

  // The delegate is responsible for executing commands.
  CommandUpdaterDelegate* delegate_;

  // This is a map of command IDs to states and observer lists
  std::unordered_map<int, std::unique_ptr<Command>> commands_;

  DISALLOW_COPY_AND_ASSIGN(CommandUpdaterImpl);
};

#endif  // CHROME_BROWSER_COMMAND_UPDATER_IMPL_H_
