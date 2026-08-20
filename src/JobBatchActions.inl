// SPDX-License-Identifier: GPL-3.0-only
// Value-only planning and fail-closed execution for permanent-job batches.
//
// Integration contract:
//   * Include after RuntimeAccess.inl, GeneralJobTransfer.inl, and
//     SquadPriority.inl.
//   * UI callbacks publish JobBatchSelectionValue and HandleIdentity values;
//     call the mutating entry points later on the game update thread.
//   * Call ClearJobBatchClipboard when the manager window closes or the world
//     resets. The clipboard never contains Tasker*, TaskData*, Character*, or
//     another borrowed engine pointer.
//   * Native append is enabled only by KJM_JOB_BATCH_ACTIONS_PROBE or, after
//     the disposable-save matrix passes, KJM_JOB_BATCH_ACTIONS_VERIFIED.

    struct JobBatchSelectionValue
    {
        // The caller supplies selections in exact visible-board order. The
        // batch core never groups or sorts this input before it captures the
        // clipboard or appends destination bundles.
        GeneralJobQueueValue sourceBefore;
        int sourceSlot;

        JobBatchSelectionValue() : sourceSlot(-1) {}
    };

    struct JobBatchBundleValue
    {
        std::vector<GeneralJobRowValue> rows;
    };

    struct JobBatchMoveBundleValue
    {
        JobBatchBundleValue payload;
    };

    struct JobBatchClipboardState
    {
        std::vector<JobBatchBundleValue> bundles;
        unsigned int revision;

        JobBatchClipboardState() : revision(0) {}
    };

    enum JobBatchActionCode
    {
        JOB_BATCH_SUCCESS,
        JOB_BATCH_NOTHING_TO_DO,
        JOB_BATCH_DISABLED,
        JOB_BATCH_INVALID_REQUEST,
        JOB_BATCH_CLIPBOARD_EMPTY,
        JOB_BATCH_CAPTURE_FAILED,
        JOB_BATCH_SOURCE_CHANGED,
        JOB_BATCH_DESTINATION_CHANGED,
        JOB_BATCH_DESTINATION_FULL,
        JOB_BATCH_DUPLICATE,
        JOB_BATCH_ADD_REJECTED,
        JOB_BATCH_ADD_UNEXPECTED_REVIEW,
        JOB_BATCH_PRIORITY_FAILED_REVIEW,
        JOB_BATCH_SOURCE_CHANGED_DUPLICATE_REMAINS,
        JOB_BATCH_REMOVE_FAILED_DUPLICATE_REMAINS,
        JOB_BATCH_ALLOCATION_FAILED_REVIEW
    };

    struct JobBatchActionOutcome
    {
        JobBatchActionCode code;
        bool interrupted;
        bool destinationChanged;
        bool sourceChanged;
        int consideredRecipients;
        int completedRecipients;
        int consideredBundles;
        int appendedBundles;
        int skippedDuplicateBundles;
        int appendedRows;
        int removedRows;
        int addedHealingJobs;
        int movedPriorityRows;
        HandleIdentity failedMember;
        int failedBundle;

        JobBatchActionOutcome() :
            code(JOB_BATCH_INVALID_REQUEST), interrupted(false),
            destinationChanged(false), sourceChanged(false),
            consideredRecipients(0), completedRecipients(0),
            consideredBundles(0), appendedBundles(0),
            skippedDuplicateBundles(0), appendedRows(0), removedRows(0),
            addedHealingJobs(0), movedPriorityRows(0), failedBundle(-1)
        {
        }
    };

    struct JobBatchSourcePlan
    {
        GeneralJobQueueValue before;
        std::vector<int> sourceSlots;
    };

    struct JobBatchRosterSquadValue
    {
        HandleIdentity squad;
        std::vector<HandleIdentity> members;
    };

    struct JobBatchActiveRosterValue
    {
        std::vector<JobBatchRosterSquadValue> squads;
    };

    JobBatchClipboardState g_jobBatchClipboard;

    void ClearJobBatchClipboard()
    {
        g_jobBatchClipboard.bundles.clear();
        ++g_jobBatchClipboard.revision;
    }

    bool HasJobBatchClipboard()
    {
        return !g_jobBatchClipboard.bundles.empty();
    }

    bool JobBatchNativeAppendEnabled()
    {
#if defined(KJM_JOB_BATCH_ACTIONS_PROBE) || \
    defined(KJM_JOB_BATCH_ACTIONS_VERIFIED)
        return true;
#else
        return false;
#endif
    }

    int FindJobBatchSourcePlan(
        const std::vector<JobBatchSourcePlan>& plans,
        const HandleIdentity& member)
    {
        for (size_t index = 0; index < plans.size(); ++index)
        {
            if (SameHandleIdentity(plans[index].before.member, member))
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    int FindJobBatchIdentity(
        const std::vector<HandleIdentity>& values,
        const HandleIdentity& wanted)
    {
        for (size_t index = 0; index < values.size(); ++index)
        {
            if (SameHandleIdentity(values[index], wanted))
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    bool TryCaptureJobBatchActiveRoster(
        JobBatchActiveRosterValue* rosterOut)
    {
        if (rosterOut == NULL)
        {
            return false;
        }
        std::vector<ActiveSquadValueSeed> seeds;
        bool incomplete = false;
        if (!BuildActiveSquadValueSeeds(&seeds, &incomplete) || incomplete)
        {
            return false;
        }

        JobBatchActiveRosterValue roster;
        roster.squads.reserve(seeds.size());
        std::vector<HandleIdentity> allMembers;
        for (size_t squadIndex = 0;
             squadIndex < seeds.size(); ++squadIndex)
        {
            if (seeds[squadIndex].incomplete ||
                !seeds[squadIndex].identity.valid)
            {
                return false;
            }
            JobBatchRosterSquadValue squad;
            squad.squad = seeds[squadIndex].identity;
            squad.members = seeds[squadIndex].members;
            for (size_t memberIndex = 0;
                 memberIndex < squad.members.size(); ++memberIndex)
            {
                if (!squad.members[memberIndex].valid ||
                    squad.members[memberIndex].type != CHARACTER ||
                    FindJobBatchIdentity(
                        allMembers, squad.members[memberIndex]) >= 0)
                {
                    return false;
                }
                allMembers.push_back(squad.members[memberIndex]);
            }
            roster.squads.push_back(squad);
        }
        *rosterOut = roster;
        return true;
    }

    bool SameJobBatchActiveRoster(
        const JobBatchActiveRosterValue& left,
        const JobBatchActiveRosterValue& right)
    {
        if (left.squads.size() != right.squads.size())
        {
            return false;
        }
        for (size_t squadIndex = 0;
             squadIndex < left.squads.size(); ++squadIndex)
        {
            if (!SameHandleIdentity(
                    left.squads[squadIndex].squad,
                    right.squads[squadIndex].squad) ||
                left.squads[squadIndex].members.size() !=
                    right.squads[squadIndex].members.size())
            {
                return false;
            }
            for (size_t memberIndex = 0;
                 memberIndex < left.squads[squadIndex].members.size();
                 ++memberIndex)
            {
                if (!SameHandleIdentity(
                        left.squads[squadIndex].members[memberIndex],
                        right.squads[squadIndex].members[memberIndex]))
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool JobBatchRosterContainsMember(
        const JobBatchActiveRosterValue& roster,
        const HandleIdentity& member)
    {
        int matches = 0;
        for (size_t squadIndex = 0;
             squadIndex < roster.squads.size(); ++squadIndex)
        {
            for (size_t memberIndex = 0;
                 memberIndex < roster.squads[squadIndex].members.size();
                 ++memberIndex)
            {
                if (SameHandleIdentity(
                        roster.squads[squadIndex].members[memberIndex],
                        member))
                {
                    ++matches;
                }
            }
        }
        return matches == 1;
    }

    bool JobBatchRosterContainsMembers(
        const JobBatchActiveRosterValue& roster,
        const std::vector<HandleIdentity>& members)
    {
        if (members.empty())
        {
            return false;
        }
        for (size_t index = 0; index < members.size(); ++index)
        {
            if (!JobBatchRosterContainsMember(roster, members[index]))
            {
                return false;
            }
        }
        return true;
    }

    bool RevalidateJobBatchActiveRoster(
        const JobBatchActiveRosterValue& expected,
        const std::vector<HandleIdentity>& requiredMembers)
    {
        JobBatchActiveRosterValue fresh;
        return TryCaptureJobBatchActiveRoster(&fresh) &&
            SameJobBatchActiveRoster(expected, fresh) &&
            JobBatchRosterContainsMembers(fresh, requiredMembers);
    }

    bool IsJobBatchCompanionPair(
        const GeneralJobRowValue& primary,
        const GeneralJobRowValue& companion)
    {
        return primary.taskData.associatedSecondary != NULL_TASK &&
            primary.taskData.associatedSecondary == companion.taskType &&
            SameHandleIdentity(
                primary.subjectIdentity, companion.subjectIdentity) &&
            SameGeneralJobLocation(primary.location, companion.location);
    }

    // Resolve a selected companion back to its primary. A malformed or
    // ambiguous companion relationship fails before any engine call.
    bool TryBuildJobBatchBundle(
        const GeneralJobQueueValue& queue,
        int selectedSlot,
        int* primarySlotOut,
        JobBatchBundleValue* bundleOut)
    {
        if (primarySlotOut == NULL || bundleOut == NULL ||
            selectedSlot < 0 ||
            selectedSlot >= static_cast<int>(queue.rows.size()))
        {
            return false;
        }

        int primarySlot = selectedSlot;
        int companionOwner = -1;
        for (size_t index = 0; index < queue.rows.size(); ++index)
        {
            if (static_cast<int>(index) == selectedSlot ||
                queue.rows[index].taskData.associatedSecondary !=
                    queue.rows[selectedSlot].taskType)
            {
                continue;
            }
            if (IsJobBatchCompanionPair(
                    queue.rows[index], queue.rows[selectedSlot]))
            {
                if (companionOwner >= 0)
                {
                    return false;
                }
                companionOwner = static_cast<int>(index);
            }
        }
        if (companionOwner >= 0)
        {
            if (companionOwner + 1 != selectedSlot)
            {
                return false;
            }
            primarySlot = companionOwner;
        }

        const GeneralJobRowValue& primary = queue.rows[primarySlot];
        const TaskType normalizedPrimary =
            primary.taskData.associated == NULL_TASK ?
            primary.taskType : primary.taskData.associated;
        if (normalizedPrimary != primary.taskType ||
            !IsValidGeneralJobRowValue(primary))
        {
            return false;
        }

        JobBatchBundleValue bundle;
        bundle.rows.push_back(primary);
        if (primary.taskData.associatedSecondary != NULL_TASK)
        {
            if (primarySlot + 1 >= static_cast<int>(queue.rows.size()) ||
                !IsJobBatchCompanionPair(
                    primary, queue.rows[primarySlot + 1]) ||
                !IsValidGeneralJobRowValue(queue.rows[primarySlot + 1]))
            {
                return false;
            }
            bundle.rows.push_back(queue.rows[primarySlot + 1]);
        }
        *primarySlotOut = primarySlot;
        *bundleOut = bundle;
        return true;
    }

    bool JobBatchContainsSlot(
        const std::vector<int>& slots,
        int wanted)
    {
        for (size_t index = 0; index < slots.size(); ++index)
        {
            if (slots[index] == wanted)
            {
                return true;
            }
        }
        return false;
    }

    bool BuildJobBatchSourcePlans(
        const std::vector<JobBatchSelectionValue>& selections,
        std::vector<JobBatchSourcePlan>* plansOut,
        std::vector<JobBatchMoveBundleValue>* bundlesOut)
    {
        if (plansOut == NULL || bundlesOut == NULL || selections.empty())
        {
            return false;
        }
        plansOut->clear();
        bundlesOut->clear();

        for (size_t index = 0; index < selections.size(); ++index)
        {
            const JobBatchSelectionValue& selection = selections[index];
            if (!selection.sourceBefore.member.valid ||
                selection.sourceBefore.member.type != CHARACTER)
            {
                return false;
            }
            int planIndex = FindJobBatchSourcePlan(
                *plansOut, selection.sourceBefore.member);
            if (planIndex < 0)
            {
                JobBatchSourcePlan plan;
                plan.before = selection.sourceBefore;
                plansOut->push_back(plan);
                planIndex = static_cast<int>(plansOut->size()) - 1;
            }
            else if (!SameGeneralJobQueue(
                         (*plansOut)[planIndex].before,
                         selection.sourceBefore))
            {
                return false;
            }

            int primarySlot = -1;
            JobBatchBundleValue payload;
            if (!TryBuildJobBatchBundle(
                    (*plansOut)[planIndex].before,
                    selection.sourceSlot, &primarySlot, &payload))
            {
                return false;
            }
            if (JobBatchContainsSlot(
                    (*plansOut)[planIndex].sourceSlots, primarySlot))
            {
                // Primary and companion can both be visibly selected. They
                // still represent one inseparable native bundle.
                continue;
            }
            (*plansOut)[planIndex].sourceSlots.push_back(primarySlot);

            JobBatchMoveBundleValue moving;
            // Preserve the first visible occurrence across squad/member
            // boundaries. Source plans are separate and affect only the later
            // reverse-slot removal phase.
            moving.payload = payload;
            bundlesOut->push_back(moving);
        }
        return !bundlesOut->empty();
    }

    bool RevalidateJobBatchSourcePlans(
        const std::vector<JobBatchSourcePlan>& plans)
    {
        for (size_t index = 0; index < plans.size(); ++index)
        {
            GeneralJobQueueValue live;
            if (!TryCaptureGeneralJobQueue(plans[index].before.member, &live) ||
                !SameGeneralJobQueue(plans[index].before, live))
            {
                return false;
            }
        }
        return true;
    }

    bool CaptureJobBatchClipboard(
        const std::vector<JobBatchSelectionValue>& selections,
        JobBatchActionOutcome* outcomeOut)
    {
        JobBatchActionOutcome local;
        JobBatchActionOutcome* outcome =
            outcomeOut == NULL ? &local : outcomeOut;
        *outcome = JobBatchActionOutcome();
        try
        {
            std::vector<JobBatchSourcePlan> plans;
            std::vector<JobBatchMoveBundleValue> moving;
            if (!BuildJobBatchSourcePlans(selections, &plans, &moving))
            {
                outcome->code = JOB_BATCH_INVALID_REQUEST;
                return false;
            }
            std::vector<HandleIdentity> sourceMembers;
            sourceMembers.reserve(plans.size());
            for (size_t index = 0; index < plans.size(); ++index)
            {
                sourceMembers.push_back(plans[index].before.member);
            }
            JobBatchActiveRosterValue expectedRoster;
            if (!TryCaptureJobBatchActiveRoster(&expectedRoster) ||
                !JobBatchRosterContainsMembers(
                    expectedRoster, sourceMembers))
            {
                outcome->code = JOB_BATCH_CAPTURE_FAILED;
                outcome->interrupted = true;
                return false;
            }
            if (!RevalidateJobBatchSourcePlans(plans))
            {
                outcome->code = JOB_BATCH_SOURCE_CHANGED;
                outcome->interrupted = true;
                return false;
            }
            if (!RevalidateJobBatchActiveRoster(
                    expectedRoster, sourceMembers))
            {
                outcome->code = JOB_BATCH_SOURCE_CHANGED;
                outcome->interrupted = true;
                return false;
            }

            std::vector<JobBatchBundleValue> copied;
            copied.reserve(moving.size());
            for (size_t index = 0; index < moving.size(); ++index)
            {
                copied.push_back(moving[index].payload);
            }
            g_jobBatchClipboard.bundles = copied;
            ++g_jobBatchClipboard.revision;
            outcome->consideredBundles = static_cast<int>(copied.size());
            outcome->code = JOB_BATCH_SUCCESS;
            return true;
        }
        catch (...)
        {
            outcome->code = JOB_BATCH_ALLOCATION_FAILED_REVIEW;
            outcome->interrupted = true;
            return false;
        }
    }

    bool JobBatchIsGlobalRole(TaskType taskType)
    {
        const TaskType role = GeneralJobSemanticRole(taskType);
        return IsGlobalGeneralJobRole(role) || role == SPLINT_JOB;
    }

    bool SameJobBatchSemantic(
        const GeneralJobRowValue& left,
        const GeneralJobRowValue& right)
    {
        if (GeneralJobSemanticRole(left.taskType) !=
            GeneralJobSemanticRole(right.taskType))
        {
            return false;
        }
        if (JobBatchIsGlobalRole(left.taskType) &&
            JobBatchIsGlobalRole(right.taskType))
        {
            return true;
        }
        return SameHandleIdentity(
            left.subjectIdentity, right.subjectIdentity);
    }

    bool HasJobBatchSemanticDuplicate(
        const std::vector<GeneralJobRowValue>& haystack,
        const JobBatchBundleValue& bundle)
    {
        for (size_t row = 0; row < bundle.rows.size(); ++row)
        {
            for (size_t index = 0; index < haystack.size(); ++index)
            {
                if (SameJobBatchSemantic(haystack[index], bundle.rows[row]))
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool ValidateJobBatchBundleSubjects(const JobBatchBundleValue& bundle)
    {
        if (bundle.rows.empty() || bundle.rows.size() > 2 ||
            !TryValidateGeneralJobSubject(&bundle.rows[0]))
        {
            return false;
        }
        const GeneralJobRowValue& primary = bundle.rows[0];
        const TaskType normalizedPrimary =
            primary.taskData.associated == NULL_TASK ?
            primary.taskType : primary.taskData.associated;
        if (normalizedPrimary != primary.taskType)
        {
            return false;
        }
        for (size_t index = 0; index < bundle.rows.size(); ++index)
        {
            if (!IsValidGeneralJobRowValue(bundle.rows[index]))
            {
                return false;
            }
        }
        if (bundle.rows.size() == 1)
        {
            return primary.taskData.associatedSecondary == NULL_TASK;
        }
        return IsJobBatchCompanionPair(primary, bundle.rows[1]);
    }

    JobBatchActionCode AppendJobBatchBundleAndVerify(
        const HandleIdentity& destination,
        const GeneralJobQueueValue& before,
        const JobBatchBundleValue& bundle,
        const JobBatchActiveRosterValue& expectedRoster,
        const std::vector<HandleIdentity>& requiredMembers,
        GeneralJobQueueValue* afterOut,
        bool* changedOut)
    {
        if (afterOut == NULL || changedOut == NULL ||
            !SameHandleIdentity(destination, before.member) ||
            !ValidateJobBatchBundleSubjects(bundle))
        {
            return JOB_BATCH_INVALID_REQUEST;
        }
        *changedOut = false;
        if (!JobBatchNativeAppendEnabled())
        {
            return JOB_BATCH_DISABLED;
        }
        if (before.rows.size() + bundle.rows.size() >
            static_cast<size_t>(MAX_SAFE_JOB_ROWS))
        {
            return JOB_BATCH_DESTINATION_FULL;
        }
        if (!RevalidateJobBatchActiveRoster(
                expectedRoster, requiredMembers))
        {
            return JOB_BATCH_DESTINATION_CHANGED;
        }
        GeneralJobQueueValue immediatelyBeforeAdd;
        if (!TryCaptureGeneralJobQueue(
                destination, &immediatelyBeforeAdd) ||
            !SameGeneralJobQueue(before, immediatelyBeforeAdd))
        {
            return JOB_BATCH_DESTINATION_CHANGED;
        }

        bool nativeReturned = false;
        TryAddGeneralJobLeaf(
            &destination, &bundle.rows[0], &nativeReturned);
        GeneralJobQueueValue after;
        if (!TryCaptureGeneralJobQueue(destination, &after))
        {
            return JOB_BATCH_ADD_UNEXPECTED_REVIEW;
        }
        if (SameGeneralJobQueue(before, after))
        {
            return JOB_BATCH_ADD_REJECTED;
        }
        *changedOut = true;
        if (!IsGeneralJobPrefix(before, after) ||
            after.rows.size() - before.rows.size() != bundle.rows.size())
        {
            return JOB_BATCH_ADD_UNEXPECTED_REVIEW;
        }
        for (size_t index = 0; index < bundle.rows.size(); ++index)
        {
            if (!SameGeneralJobRecreatedPayload(
                    bundle.rows[index],
                    after.rows[before.rows.size() + index]))
            {
                return JOB_BATCH_ADD_UNEXPECTED_REVIEW;
            }
        }
        *afterOut = after;
        (void)nativeReturned;
        return JOB_BATCH_SUCCESS;
    }

    bool BuildUniqueJobBatchRecipients(
        const std::vector<HandleIdentity>& requested,
        std::vector<HandleIdentity>* uniqueOut)
    {
        if (uniqueOut == NULL || requested.empty())
        {
            return false;
        }
        uniqueOut->clear();
        for (size_t index = 0; index < requested.size(); ++index)
        {
            if (!requested[index].valid ||
                requested[index].type != CHARACTER)
            {
                return false;
            }
            if (FindJobBatchIdentity(*uniqueOut, requested[index]) < 0)
            {
                uniqueOut->push_back(requested[index]);
            }
        }
        return !uniqueOut->empty();
    }

    struct JobBatchPasteRecipientPlan
    {
        GeneralJobQueueValue before;
        std::vector<size_t> appendBundleIndexes;
        int skippedDuplicates;

        JobBatchPasteRecipientPlan() : skippedDuplicates(0) {}
    };

    bool PasteJobBatchClipboard(
        const std::vector<HandleIdentity>& recipients,
        JobBatchActionOutcome* outcomeOut)
    {
        JobBatchActionOutcome local;
        JobBatchActionOutcome* outcome =
            outcomeOut == NULL ? &local : outcomeOut;
        *outcome = JobBatchActionOutcome();
        try
        {
            if (g_jobBatchClipboard.bundles.empty())
            {
                outcome->code = JOB_BATCH_CLIPBOARD_EMPTY;
                return false;
            }
            for (size_t bundleIndex = 0;
                 bundleIndex < g_jobBatchClipboard.bundles.size();
                 ++bundleIndex)
            {
                if (!ValidateJobBatchBundleSubjects(
                        g_jobBatchClipboard.bundles[bundleIndex]))
                {
                    outcome->code = JOB_BATCH_INVALID_REQUEST;
                    outcome->failedBundle =
                        static_cast<int>(bundleIndex);
                    return false;
                }
            }
            std::vector<HandleIdentity> unique;
            if (!BuildUniqueJobBatchRecipients(recipients, &unique))
            {
                outcome->code = JOB_BATCH_INVALID_REQUEST;
                return false;
            }
            JobBatchActiveRosterValue expectedRoster;
            if (!TryCaptureJobBatchActiveRoster(&expectedRoster) ||
                !JobBatchRosterContainsMembers(expectedRoster, unique))
            {
                outcome->code = JOB_BATCH_CAPTURE_FAILED;
                outcome->interrupted = true;
                return false;
            }

            // Complete every predictable recipient check before the first
            // native append. The virtual rows include earlier planned
            // clipboard bundles, so semantic duplicates and final capacity
            // match the later append order exactly.
            std::vector<JobBatchPasteRecipientPlan> plans;
            plans.reserve(unique.size());
            for (size_t index = 0; index < unique.size(); ++index)
            {
                JobBatchPasteRecipientPlan plan;
                if (!TryCaptureGeneralJobQueue(
                        unique[index], &plan.before))
                {
                    outcome->code = JOB_BATCH_CAPTURE_FAILED;
                    outcome->failedMember = unique[index];
                    outcome->interrupted = true;
                    return false;
                }
                std::vector<GeneralJobRowValue> virtualRows =
                    plan.before.rows;
                for (size_t bundleIndex = 0;
                     bundleIndex < g_jobBatchClipboard.bundles.size();
                     ++bundleIndex)
                {
                    const JobBatchBundleValue& bundle =
                        g_jobBatchClipboard.bundles[bundleIndex];
                    if (HasJobBatchSemanticDuplicate(virtualRows, bundle))
                    {
                        ++plan.skippedDuplicates;
                        continue;
                    }
                    if (virtualRows.size() + bundle.rows.size() >
                        static_cast<size_t>(MAX_SAFE_JOB_ROWS))
                    {
                        outcome->code = JOB_BATCH_DESTINATION_FULL;
                        outcome->failedMember = unique[index];
                        outcome->failedBundle =
                            static_cast<int>(bundleIndex);
                        return false;
                    }
                    plan.appendBundleIndexes.push_back(bundleIndex);
                    virtualRows.insert(
                        virtualRows.end(), bundle.rows.begin(),
                        bundle.rows.end());
                }
                plans.push_back(plan);
            }

            for (size_t recipient = 0; recipient < unique.size(); ++recipient)
            {
                ++outcome->consideredRecipients;
                GeneralJobQueueValue working;
                if (!TryCaptureGeneralJobQueue(unique[recipient], &working) ||
                    !SameGeneralJobQueue(plans[recipient].before, working))
                {
                    outcome->code = JOB_BATCH_DESTINATION_CHANGED;
                    outcome->failedMember = unique[recipient];
                    outcome->interrupted = true;
                    return false;
                }
                outcome->consideredBundles += static_cast<int>(
                    g_jobBatchClipboard.bundles.size());
                outcome->skippedDuplicateBundles +=
                    plans[recipient].skippedDuplicates;
                for (size_t planIndex = 0;
                     planIndex <
                        plans[recipient].appendBundleIndexes.size();
                     ++planIndex)
                {
                    const size_t bundleIndex =
                        plans[recipient].appendBundleIndexes[planIndex];
                    const JobBatchBundleValue& bundle =
                        g_jobBatchClipboard.bundles[bundleIndex];

                    GeneralJobQueueValue after;
                    bool changed = false;
                    const JobBatchActionCode appendCode =
                        AppendJobBatchBundleAndVerify(
                            unique[recipient], working, bundle,
                            expectedRoster, unique,
                            &after, &changed);
                    if (changed)
                    {
                        outcome->destinationChanged = true;
                    }
                    if (appendCode != JOB_BATCH_SUCCESS)
                    {
                        outcome->code = appendCode;
                        outcome->failedMember = unique[recipient];
                        outcome->failedBundle =
                            static_cast<int>(bundleIndex);
                        outcome->interrupted = true;
                        return false;
                    }
                    working = after;
                    ++outcome->appendedBundles;
                    outcome->appendedRows +=
                        static_cast<int>(bundle.rows.size());
                }
                ++outcome->completedRecipients;
            }
            outcome->code = outcome->appendedBundles == 0 ?
                JOB_BATCH_NOTHING_TO_DO : JOB_BATCH_SUCCESS;
            return true;
        }
        catch (...)
        {
            outcome->code = JOB_BATCH_ALLOCATION_FAILED_REVIEW;
            outcome->interrupted = true;
            return false;
        }
    }

    bool JobBatchSlotGreater(int left, int right)
    {
        return left > right;
    }

    bool MoveSelectedJobBatch(
        const std::vector<JobBatchSelectionValue>& selections,
        const HandleIdentity& destination,
        JobBatchActionOutcome* outcomeOut)
    {
        JobBatchActionOutcome local;
        JobBatchActionOutcome* outcome =
            outcomeOut == NULL ? &local : outcomeOut;
        *outcome = JobBatchActionOutcome();
        try
        {
            if (!destination.valid || destination.type != CHARACTER)
            {
                outcome->code = JOB_BATCH_INVALID_REQUEST;
                return false;
            }
            std::vector<JobBatchSourcePlan> plans;
            std::vector<JobBatchMoveBundleValue> moving;
            if (!BuildJobBatchSourcePlans(selections, &plans, &moving))
            {
                outcome->code = JOB_BATCH_INVALID_REQUEST;
                return false;
            }
            if (FindJobBatchSourcePlan(plans, destination) >= 0)
            {
                // A batch never combines same-member reorder with
                // cross-member transfer.
                outcome->code = JOB_BATCH_INVALID_REQUEST;
                outcome->failedMember = destination;
                return false;
            }
            std::vector<HandleIdentity> participants;
            participants.reserve(plans.size() + 1);
            for (size_t index = 0; index < plans.size(); ++index)
            {
                participants.push_back(plans[index].before.member);
            }
            participants.push_back(destination);
            JobBatchActiveRosterValue expectedRoster;
            if (!TryCaptureJobBatchActiveRoster(&expectedRoster) ||
                !JobBatchRosterContainsMembers(
                    expectedRoster, participants))
            {
                outcome->code = JOB_BATCH_CAPTURE_FAILED;
                outcome->interrupted = true;
                return false;
            }
            if (!RevalidateJobBatchSourcePlans(plans))
            {
                outcome->code = JOB_BATCH_SOURCE_CHANGED;
                outcome->interrupted = true;
                return false;
            }

            GeneralJobQueueValue destinationBefore;
            if (!TryCaptureGeneralJobQueue(
                    destination, &destinationBefore))
            {
                outcome->code = JOB_BATCH_CAPTURE_FAILED;
                outcome->failedMember = destination;
                outcome->interrupted = true;
                return false;
            }
            std::vector<GeneralJobRowValue> plannedRows =
                destinationBefore.rows;
            for (size_t index = 0; index < moving.size(); ++index)
            {
                if (HasJobBatchSemanticDuplicate(
                        plannedRows, moving[index].payload))
                {
                    outcome->code = JOB_BATCH_DUPLICATE;
                    outcome->failedBundle = static_cast<int>(index);
                    return false;
                }
                if (!ValidateJobBatchBundleSubjects(moving[index].payload))
                {
                    outcome->code = JOB_BATCH_CAPTURE_FAILED;
                    outcome->failedBundle = static_cast<int>(index);
                    return false;
                }
                plannedRows.insert(
                    plannedRows.end(), moving[index].payload.rows.begin(),
                    moving[index].payload.rows.end());
            }
            if (plannedRows.size() > static_cast<size_t>(MAX_SAFE_JOB_ROWS))
            {
                outcome->code = JOB_BATCH_DESTINATION_FULL;
                return false;
            }

            GeneralJobQueueValue destinationWorking = destinationBefore;
            for (size_t index = 0; index < moving.size(); ++index)
            {
                ++outcome->consideredBundles;
                GeneralJobQueueValue after;
                bool changed = false;
                const JobBatchActionCode appendCode =
                    AppendJobBatchBundleAndVerify(
                        destination, destinationWorking,
                        moving[index].payload,
                        expectedRoster, participants,
                        &after, &changed);
                if (changed)
                {
                    outcome->destinationChanged = true;
                }
                if (appendCode != JOB_BATCH_SUCCESS)
                {
                    outcome->code = appendCode;
                    outcome->failedMember = destination;
                    outcome->failedBundle = static_cast<int>(index);
                    outcome->interrupted = true;
                    return false;
                }
                destinationWorking = after;
                ++outcome->appendedBundles;
                outcome->appendedRows += static_cast<int>(
                    moving[index].payload.rows.size());
            }

            // All destination copies now exist. Revalidate every source and
            // the complete destination before the first destructive remove.
            if (!RevalidateJobBatchSourcePlans(plans))
            {
                outcome->code =
                    JOB_BATCH_SOURCE_CHANGED_DUPLICATE_REMAINS;
                outcome->interrupted = true;
                return false;
            }
            if (!RevalidateJobBatchActiveRoster(
                    expectedRoster, participants))
            {
                outcome->code =
                    JOB_BATCH_SOURCE_CHANGED_DUPLICATE_REMAINS;
                outcome->interrupted = true;
                return false;
            }
            GeneralJobQueueValue destinationCheck;
            if (!TryCaptureGeneralJobQueue(destination, &destinationCheck) ||
                !SameGeneralJobQueue(
                    destinationWorking, destinationCheck))
            {
                outcome->code = JOB_BATCH_DESTINATION_CHANGED;
                outcome->interrupted = true;
                return false;
            }

            // Slots are meaningful only within one source queue. Remove
            // source plans in reverse encounter order and each plan's bundles
            // in reverse slot order. Companion rows are removed tail first.
            for (size_t planOffset = plans.size(); planOffset > 0; --planOffset)
            {
                JobBatchSourcePlan plan = plans[planOffset - 1];
                std::sort(
                    plan.sourceSlots.begin(), plan.sourceSlots.end(),
                    JobBatchSlotGreater);
                GeneralJobQueueValue sourceWorking;
                if (!TryCaptureGeneralJobQueue(
                        plan.before.member, &sourceWorking) ||
                    !SameGeneralJobQueue(plan.before, sourceWorking))
                {
                    outcome->code =
                        JOB_BATCH_SOURCE_CHANGED_DUPLICATE_REMAINS;
                    outcome->failedMember = plan.before.member;
                    outcome->interrupted = true;
                    return false;
                }

                for (size_t slotIndex = 0;
                     slotIndex < plan.sourceSlots.size(); ++slotIndex)
                {
                    const int primarySlot = plan.sourceSlots[slotIndex];
                    int verifiedPrimary = -1;
                    JobBatchBundleValue sourceBundle;
                    if (!TryBuildJobBatchBundle(
                            sourceWorking, primarySlot,
                            &verifiedPrimary, &sourceBundle) ||
                        verifiedPrimary != primarySlot)
                    {
                        outcome->code =
                            JOB_BATCH_REMOVE_FAILED_DUPLICATE_REMAINS;
                        outcome->failedMember = plan.before.member;
                        outcome->interrupted = true;
                        return false;
                    }

                    for (size_t rowOffset = sourceBundle.rows.size();
                         rowOffset > 0; --rowOffset)
                    {
                        GeneralJobQueueValue destinationStill;
                        if (!TryCaptureGeneralJobQueue(
                                destination, &destinationStill) ||
                            !SameGeneralJobQueue(
                                destinationWorking, destinationStill))
                        {
                            outcome->code =
                                JOB_BATCH_REMOVE_FAILED_DUPLICATE_REMAINS;
                            outcome->failedMember = destination;
                            outcome->interrupted = true;
                            return false;
                        }
                        const int removeSlot = primarySlot +
                            static_cast<int>(rowOffset - 1);
                        if (removeSlot < 0 ||
                            removeSlot >=
                                static_cast<int>(sourceWorking.rows.size()) ||
                            !SameGeneralJobRowStructure(
                                sourceBundle.rows[rowOffset - 1],
                                sourceWorking.rows[removeSlot]))
                        {
                            outcome->code =
                                JOB_BATCH_REMOVE_FAILED_DUPLICATE_REMAINS;
                            outcome->failedMember = plan.before.member;
                            outcome->interrupted = true;
                            return false;
                        }
                        if (!RevalidateJobBatchActiveRoster(
                                expectedRoster, participants))
                        {
                            outcome->code =
                                JOB_BATCH_REMOVE_FAILED_DUPLICATE_REMAINS;
                            outcome->failedMember = plan.before.member;
                            outcome->interrupted = true;
                            return false;
                        }
                        GeneralJobQueueValue immediatelyBeforeRemove;
                        if (!TryCaptureGeneralJobQueue(
                                plan.before.member,
                                &immediatelyBeforeRemove) ||
                            !SameGeneralJobQueue(
                                sourceWorking,
                                immediatelyBeforeRemove))
                        {
                            outcome->code =
                                JOB_BATCH_REMOVE_FAILED_DUPLICATE_REMAINS;
                            outcome->failedMember = plan.before.member;
                            outcome->interrupted = true;
                            return false;
                        }
                        GeneralJobQueueValue afterRemove;
                        if (!TryRemoveGeneralJobAndVerify(
                                plan.before.member, sourceWorking,
                                removeSlot, &afterRemove))
                        {
                            outcome->code =
                                JOB_BATCH_REMOVE_FAILED_DUPLICATE_REMAINS;
                            outcome->failedMember = plan.before.member;
                            outcome->interrupted = true;
                            return false;
                        }
                        sourceWorking = afterRemove;
                        outcome->sourceChanged = true;
                        ++outcome->removedRows;
                    }
                }
            }

            if (!TryCaptureGeneralJobQueue(destination, &destinationCheck) ||
                !SameGeneralJobQueue(destinationWorking, destinationCheck))
            {
                outcome->code =
                    JOB_BATCH_REMOVE_FAILED_DUPLICATE_REMAINS;
                outcome->interrupted = true;
                return false;
            }
            outcome->completedRecipients = 1;
            outcome->code = JOB_BATCH_SUCCESS;
            return true;
        }
        catch (...)
        {
            outcome->code = JOB_BATCH_ALLOCATION_FAILED_REVIEW;
            outcome->interrupted = true;
            return false;
        }
    }

    enum JobBatchHealingRole
    {
        JOB_BATCH_HEAL_RESCUE = 0,
        JOB_BATCH_HEAL_PUT_IN_BED = 1,
        JOB_BATCH_HEAL_MEDIC = 2,
        JOB_BATCH_HEAL_ROBOTICS = 3,
        JOB_BATCH_HEAL_SPLINTING = 4,
        JOB_BATCH_HEAL_ENGINEERING = 5,
        JOB_BATCH_HEAL_NONE = 6
    };

    int GetJobBatchHealingRole(TaskType taskType)
    {
        const TaskType role = GeneralJobSemanticRole(taskType);
        if (role == FIND_AND_RESCUE)
        {
            return JOB_BATCH_HEAL_RESCUE;
        }
        if (role == FIND_BED_AND_PUT_IN)
        {
            return JOB_BATCH_HEAL_PUT_IN_BED;
        }
        if (role == JOB_MEDIC)
        {
            return JOB_BATCH_HEAL_MEDIC;
        }
        if (role == JOB_REPAIR_ROBOT)
        {
            return JOB_BATCH_HEAL_ROBOTICS;
        }
        if (role == SPLINT_JOB)
        {
            return JOB_BATCH_HEAL_SPLINTING;
        }
        if (role == JOB_BUILDER)
        {
            return JOB_BATCH_HEAL_ENGINEERING;
        }
        return JOB_BATCH_HEAL_NONE;
    }

    TaskType JobBatchHealingTaskType(int role)
    {
        switch (role)
        {
        case JOB_BATCH_HEAL_RESCUE:
            // FIND_AND_RESCUE is the user-requested task. Kenshi can publish
            // FIND_AND_RESCUE_IF_THERES_BEDS as its live permanent wrapper;
            // GeneralJobSemanticRole deliberately verifies both as Rescue.
            return FIND_AND_RESCUE;
        case JOB_BATCH_HEAL_PUT_IN_BED:
            return FIND_BED_AND_PUT_IN;
        case JOB_BATCH_HEAL_MEDIC:
            return JOB_MEDIC;
        case JOB_BATCH_HEAL_ROBOTICS:
            return JOB_REPAIR_ROBOT;
        case JOB_BATCH_HEAL_SPLINTING:
            return SPLINT_JOB;
        case JOB_BATCH_HEAL_ENGINEERING:
            return JOB_BUILDER;
        default:
            return NULL_TASK;
        }
    }

    bool HasJobBatchHealingRole(
        const GeneralJobQueueValue& queue,
        int wantedRole)
    {
        for (size_t index = 0; index < queue.rows.size(); ++index)
        {
            if (GetJobBatchHealingRole(queue.rows[index].taskType) ==
                wantedRole)
            {
                return true;
            }
        }
        return false;
    }

    __declspec(noinline) bool AddJobBatchHealingCppUnchecked(
        const HandleIdentity& destination,
        TaskType taskType)
    {
        if (!destination.valid || destination.type != CHARACTER ||
            taskType == NULL_TASK)
        {
            return false;
        }
        const hand member = RestoreHandleIdentity(destination);
        if (!member || !member.isValid())
        {
            return false;
        }
        Character* character = member.getCharacter();
        if (character == NULL || !character->isPlayerCharacter())
        {
            return false;
        }
        Faction* faction = character->getFaction();
        OrdersReceiver* receiver = character->getOrdersReciever();
        if (faction == NULL || !faction->isThePlayer() || receiver == NULL)
        {
            return false;
        }
        hand nullSubject;
        receiver->addJob(
            taskType, nullSubject,
            Ogre::Vector3(0.0f, 0.0f, 0.0f), true);
        character->reThinkCurrentAIAction();
        return true;
    }

    __declspec(noinline) bool AddJobBatchHealingCpp(
        const HandleIdentity& destination,
        TaskType taskType)
    {
        try
        {
            return AddJobBatchHealingCppUnchecked(destination, taskType);
        }
        catch (...)
        {
            return false;
        }
    }

    __declspec(noinline) bool TryAddJobBatchHealingLeaf(
        const HandleIdentity* destination,
        TaskType taskType,
        bool* returnedOut)
    {
        if (returnedOut != NULL)
        {
            *returnedOut = false;
        }
        __try
        {
            const bool result = destination != NULL &&
                AddJobBatchHealingCpp(*destination, taskType);
            if (returnedOut != NULL)
            {
                *returnedOut = result;
            }
            return result;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    JobBatchActionCode AddMissingJobBatchHealingRole(
        const HandleIdentity& member,
        int role,
        const GeneralJobQueueValue& before,
        const JobBatchActiveRosterValue& expectedRoster,
        const std::vector<HandleIdentity>& requiredMembers,
        GeneralJobQueueValue* afterOut,
        bool* changedOut)
    {
        if (afterOut == NULL || changedOut == NULL ||
            role < JOB_BATCH_HEAL_RESCUE ||
            role > JOB_BATCH_HEAL_SPLINTING)
        {
            return JOB_BATCH_INVALID_REQUEST;
        }
        *changedOut = false;
        if (HasJobBatchHealingRole(before, role))
        {
            *afterOut = before;
            return JOB_BATCH_NOTHING_TO_DO;
        }
        if (!JobBatchNativeAppendEnabled())
        {
            return JOB_BATCH_DISABLED;
        }
        if (before.rows.size() >= static_cast<size_t>(MAX_SAFE_JOB_ROWS))
        {
            return JOB_BATCH_DESTINATION_FULL;
        }
        if (!RevalidateJobBatchActiveRoster(
                expectedRoster, requiredMembers))
        {
            return JOB_BATCH_DESTINATION_CHANGED;
        }
        GeneralJobQueueValue immediatelyBeforeAdd;
        if (!TryCaptureGeneralJobQueue(member, &immediatelyBeforeAdd) ||
            !SameGeneralJobQueue(before, immediatelyBeforeAdd))
        {
            return JOB_BATCH_DESTINATION_CHANGED;
        }

        bool nativeReturned = false;
        TryAddJobBatchHealingLeaf(
            &member, JobBatchHealingTaskType(role), &nativeReturned);
        GeneralJobQueueValue after;
        if (!TryCaptureGeneralJobQueue(member, &after))
        {
            return JOB_BATCH_ADD_UNEXPECTED_REVIEW;
        }
        if (SameGeneralJobQueue(before, after))
        {
            return JOB_BATCH_ADD_REJECTED;
        }
        *changedOut = true;
        if (!IsGeneralJobPrefix(before, after))
        {
            return JOB_BATCH_ADD_UNEXPECTED_REVIEW;
        }
        const size_t addedCount = after.rows.size() - before.rows.size();
        if (addedCount < 1 || addedCount > 2)
        {
            return JOB_BATCH_ADD_UNEXPECTED_REVIEW;
        }
        const GeneralJobRowValue& primary =
            after.rows[before.rows.size()];
        if (!IsValidGeneralJobRowValue(primary) ||
            GetJobBatchHealingRole(primary.taskType) != role)
        {
            return JOB_BATCH_ADD_UNEXPECTED_REVIEW;
        }
        if (addedCount == 1)
        {
            if (primary.taskData.associatedSecondary != NULL_TASK)
            {
                // The primary declares a companion which Kenshi did not add.
                return JOB_BATCH_ADD_UNEXPECTED_REVIEW;
            }
        }
        else
        {
            const GeneralJobRowValue& companion =
                after.rows[before.rows.size() + 1];
            if (!IsValidGeneralJobRowValue(companion) ||
                !IsJobBatchCompanionPair(primary, companion))
            {
                // Never accept an unrelated second suffix row as part of the
                // healing add that this call initiated.
                return JOB_BATCH_ADD_UNEXPECTED_REVIEW;
            }
        }
        *afterOut = after;
        (void)nativeReturned;
        return JOB_BATCH_SUCCESS;
    }

    struct JobBatchPriorityBundleValue
    {
        int role;
        std::vector<GeneralJobRowValue> rows;

        JobBatchPriorityBundleValue() : role(JOB_BATCH_HEAL_NONE) {}
    };

    bool BuildJobBatchPriorityBundles(
        const GeneralJobQueueValue& source,
        std::vector<JobBatchPriorityBundleValue>* bundlesOut)
    {
        if (bundlesOut == NULL)
        {
            return false;
        }
        bundlesOut->clear();
        size_t slot = 0;
        while (slot < source.rows.size())
        {
            // A companion must be adjacent to exactly one primary. Detect a
            // separated or multiply-owned companion before sorting rows.
            int companionOwner = -1;
            for (size_t candidate = 0;
                 candidate < source.rows.size(); ++candidate)
            {
                if (candidate == slot ||
                    !IsJobBatchCompanionPair(
                        source.rows[candidate], source.rows[slot]))
                {
                    continue;
                }
                if (companionOwner >= 0)
                {
                    return false;
                }
                companionOwner = static_cast<int>(candidate);
            }
            if (companionOwner >= 0)
            {
                // The adjacent primary was consumed during the previous
                // iteration. Reaching its companion independently is invalid.
                return false;
            }

            JobBatchPriorityBundleValue bundle;
            bundle.role = GetJobBatchHealingRole(
                source.rows[slot].taskType);
            bundle.rows.push_back(source.rows[slot]);
            if (source.rows[slot].taskData.associatedSecondary != NULL_TASK)
            {
                if (slot + 1 >= source.rows.size() ||
                    !IsJobBatchCompanionPair(
                        source.rows[slot], source.rows[slot + 1]))
                {
                    return false;
                }
                bundle.rows.push_back(source.rows[slot + 1]);
                slot += 2;
            }
            else
            {
                ++slot;
            }
            bundlesOut->push_back(bundle);
        }
        return true;
    }

    bool BuildJobBatchHealingPriorityQueue(
        const GeneralJobQueueValue& source,
        GeneralJobQueueValue* desiredOut)
    {
        if (desiredOut == NULL)
        {
            return false;
        }
        GeneralJobQueueValue desired;
        desired.member = source.member;
        desired.rows.reserve(source.rows.size());
        std::vector<JobBatchPriorityBundleValue> bundles;
        if (!BuildJobBatchPriorityBundles(source, &bundles))
        {
            return false;
        }
        for (int role = JOB_BATCH_HEAL_RESCUE;
             role <= JOB_BATCH_HEAL_ENGINEERING; ++role)
        {
            for (size_t index = 0; index < bundles.size(); ++index)
            {
                if (bundles[index].role == role)
                {
                    desired.rows.insert(
                        desired.rows.end(), bundles[index].rows.begin(),
                        bundles[index].rows.end());
                }
            }
        }
        for (size_t index = 0; index < bundles.size(); ++index)
        {
            if (bundles[index].role == JOB_BATCH_HEAL_NONE)
            {
                desired.rows.insert(
                    desired.rows.end(), bundles[index].rows.begin(),
                    bundles[index].rows.end());
            }
        }
        if (desired.rows.size() != source.rows.size())
        {
            return false;
        }
        *desiredOut = desired;
        return true;
    }

    int FindJobBatchPriorityRow(
        const std::vector<GeneralJobRowValue>& rows,
        const GeneralJobRowValue& wanted,
        int minimumSlot)
    {
        for (size_t index = static_cast<size_t>(minimumSlot);
             index < rows.size(); ++index)
        {
            if (SameGeneralJobRowStructure(rows[index], wanted))
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    JobBatchActionCode PrioritizeJobBatchHealingQueue(
        const HandleIdentity& member,
        const GeneralJobQueueValue& before,
        const JobBatchActiveRosterValue& expectedRoster,
        const std::vector<HandleIdentity>& requiredMembers,
        GeneralJobQueueValue* afterOut,
        int* movedOut)
    {
        if (afterOut == NULL || movedOut == NULL ||
            !SameHandleIdentity(member, before.member))
        {
            return JOB_BATCH_INVALID_REQUEST;
        }
        *movedOut = 0;
        GeneralJobQueueValue desired;
        if (!BuildJobBatchHealingPriorityQueue(before, &desired))
        {
            return JOB_BATCH_CAPTURE_FAILED;
        }
        GeneralJobQueueValue working = before;
        for (size_t destination = 0;
             destination < desired.rows.size(); ++destination)
        {
            if (SameGeneralJobRowStructure(
                    working.rows[destination], desired.rows[destination]))
            {
                continue;
            }
            const int source = FindJobBatchPriorityRow(
                working.rows, desired.rows[destination],
                static_cast<int>(destination) + 1);
            if (source < 0)
            {
                return JOB_BATCH_PRIORITY_FAILED_REVIEW;
            }
            if (!RevalidateJobBatchActiveRoster(
                    expectedRoster, requiredMembers))
            {
                return JOB_BATCH_DESTINATION_CHANGED;
            }
            GeneralJobQueueValue immediatelyBeforeMove;
            if (!TryCaptureGeneralJobQueue(
                    member, &immediatelyBeforeMove) ||
                !SameGeneralJobQueue(working, immediatelyBeforeMove))
            {
                return JOB_BATCH_DESTINATION_CHANGED;
            }
            GeneralJobQueueValue afterMove;
            if (!TryMoveGeneralJobAndVerify(
                    member, working, source,
                    static_cast<int>(destination), &afterMove))
            {
                return JOB_BATCH_PRIORITY_FAILED_REVIEW;
            }
            working = afterMove;
            ++(*movedOut);
        }
        if (!SameGeneralJobQueue(desired, working))
        {
            return JOB_BATCH_PRIORITY_FAILED_REVIEW;
        }
        *afterOut = working;
        return *movedOut == 0 ?
            JOB_BATCH_NOTHING_TO_DO : JOB_BATCH_SUCCESS;
    }

    // addMissing=false implements the standalone Prioritize Healing button.
    // addMissing=true implements Add Healing Jobs: add the five missing roles
    // first, then apply the same six-role priority order.
    bool ApplyJobBatchHealing(
        const std::vector<HandleIdentity>& recipients,
        bool addMissing,
        JobBatchActionOutcome* outcomeOut)
    {
        JobBatchActionOutcome local;
        JobBatchActionOutcome* outcome =
            outcomeOut == NULL ? &local : outcomeOut;
        *outcome = JobBatchActionOutcome();
        try
        {
            std::vector<HandleIdentity> unique;
            if (!BuildUniqueJobBatchRecipients(recipients, &unique))
            {
                outcome->code = JOB_BATCH_INVALID_REQUEST;
                return false;
            }
            JobBatchActiveRosterValue expectedRoster;
            if (!TryCaptureJobBatchActiveRoster(&expectedRoster) ||
                !JobBatchRosterContainsMembers(expectedRoster, unique))
            {
                outcome->code = JOB_BATCH_CAPTURE_FAILED;
                outcome->interrupted = true;
                return false;
            }
            bool anyChange = false;
            for (size_t recipient = 0; recipient < unique.size(); ++recipient)
            {
                ++outcome->consideredRecipients;
                GeneralJobQueueValue working;
                if (!TryCaptureGeneralJobQueue(unique[recipient], &working))
                {
                    outcome->code = JOB_BATCH_CAPTURE_FAILED;
                    outcome->failedMember = unique[recipient];
                    outcome->interrupted = true;
                    return false;
                }

                if (addMissing)
                {
                    for (int role = JOB_BATCH_HEAL_RESCUE;
                         role <= JOB_BATCH_HEAL_SPLINTING; ++role)
                    {
                        GeneralJobQueueValue afterAdd;
                        bool changed = false;
                        const JobBatchActionCode addCode =
                            AddMissingJobBatchHealingRole(
                                unique[recipient], role, working,
                                expectedRoster, unique,
                                &afterAdd, &changed);
                        if (changed)
                        {
                            outcome->destinationChanged = true;
                        }
                        if (addCode != JOB_BATCH_SUCCESS &&
                            addCode != JOB_BATCH_NOTHING_TO_DO)
                        {
                            outcome->code = addCode;
                            outcome->failedMember = unique[recipient];
                            outcome->interrupted = true;
                            return false;
                        }
                        if (changed)
                        {
                            working = afterAdd;
                            ++outcome->addedHealingJobs;
                            anyChange = true;
                        }
                    }
                }

                GeneralJobQueueValue prioritized;
                int moved = 0;
                const JobBatchActionCode priorityCode =
                    PrioritizeJobBatchHealingQueue(
                        unique[recipient], working,
                        expectedRoster, unique,
                        &prioritized, &moved);
                if (priorityCode != JOB_BATCH_SUCCESS &&
                    priorityCode != JOB_BATCH_NOTHING_TO_DO)
                {
                    outcome->code = priorityCode;
                    outcome->failedMember = unique[recipient];
                    outcome->interrupted = true;
                    return false;
                }
                if (moved != 0)
                {
                    outcome->destinationChanged = true;
                    outcome->movedPriorityRows += moved;
                    anyChange = true;
                }
                ++outcome->completedRecipients;
            }
            outcome->code = anyChange ?
                JOB_BATCH_SUCCESS : JOB_BATCH_NOTHING_TO_DO;
            return true;
        }
        catch (...)
        {
            outcome->code = JOB_BATCH_ALLOCATION_FAILED_REVIEW;
            outcome->interrupted = true;
            return false;
        }
    }

    std::string BuildJobBatchActionMessage(
        const JobBatchActionOutcome& outcome,
        const char* completedAction)
    {
        std::ostringstream message;
        switch (outcome.code)
        {
        case JOB_BATCH_SUCCESS:
            message << (completedAction == NULL ?
                "Job action completed." : completedAction);
            break;
        case JOB_BATCH_NOTHING_TO_DO:
            message << "No jobs needed changes.";
            break;
        case JOB_BATCH_DISABLED:
            message << "This job action is disabled in the current build.";
            break;
        case JOB_BATCH_CLIPBOARD_EMPTY:
            message << "Copy one or more jobs before pasting.";
            break;
        case JOB_BATCH_DUPLICATE:
            message << "The destination already has one of the selected jobs. No jobs were moved.";
            break;
        case JOB_BATCH_DESTINATION_FULL:
            message << "A destination job queue is at the safety limit.";
            break;
        case JOB_BATCH_SOURCE_CHANGED:
        case JOB_BATCH_DESTINATION_CHANGED:
            message << "A job queue changed before the action completed. Review the affected members and try again.";
            break;
        case JOB_BATCH_SOURCE_CHANGED_DUPLICATE_REMAINS:
        case JOB_BATCH_REMOVE_FAILED_DUPLICATE_REMAINS:
            message << "The move stopped after destination copies were added. Review the affected job queues before another change.";
            break;
        case JOB_BATCH_ADD_UNEXPECTED_REVIEW:
        case JOB_BATCH_PRIORITY_FAILED_REVIEW:
        case JOB_BATCH_ALLOCATION_FAILED_REVIEW:
            message << "The job action stopped after an unverified result. Review the affected job queues before another change.";
            break;
        case JOB_BATCH_CAPTURE_FAILED:
            message << "A selected job queue could not be read safely. No further jobs were changed.";
            break;
        case JOB_BATCH_ADD_REJECTED:
            message << "Kenshi rejected a permanent job. No further jobs were changed.";
            break;
        default:
            message << "The selected job action is no longer valid.";
            break;
        }

        if (outcome.appendedBundles != 0)
        {
            message << " Appended " << outcome.appendedBundles <<
                " job bundle" <<
                (outcome.appendedBundles == 1 ? "" : "s") << ".";
        }
        if (outcome.skippedDuplicateBundles != 0)
        {
            message << " Skipped " << outcome.skippedDuplicateBundles <<
                " duplicate" <<
                (outcome.skippedDuplicateBundles == 1 ? "" : "s") << ".";
        }
        if (outcome.addedHealingJobs != 0)
        {
            message << " Added " << outcome.addedHealingJobs <<
                " healing job" <<
                (outcome.addedHealingJobs == 1 ? "" : "s") << ".";
        }
        if (outcome.movedPriorityRows != 0)
        {
            message << " Reordered " << outcome.movedPriorityRows <<
                " job" <<
                (outcome.movedPriorityRows == 1 ? "" : "s") << ".";
        }
        if (outcome.removedRows != 0)
        {
            message << " Removed " << outcome.removedRows <<
                " source row" <<
                (outcome.removedRows == 1 ? "" : "s") << ".";
        }
        if (outcome.interrupted && outcome.completedRecipients != 0)
        {
            message << " Completed " << outcome.completedRecipients <<
                " recipient" <<
                (outcome.completedRecipients == 1 ? "" : "s") <<
                " before stopping.";
        }
        if (outcome.interrupted &&
            (outcome.destinationChanged || outcome.sourceChanged))
        {
            message << " Verified earlier changes remain; no rollback was attempted.";
        }
        return message.str();
    }
