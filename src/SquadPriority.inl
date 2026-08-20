// SPDX-License-Identifier: GPL-3.0-only
// Squad-wide permanent-job priority action.
//
// This file is intentionally self-contained at the integration boundary. It
// is included after RuntimeAccess.inl, so it can use the value-only snapshot
// types and guarded leaves already defined there. It does not retain an
// engine-owned pointer and does not perform any UI work.

    enum SquadPriorityRole
    {
        SQUAD_PRIORITY_NONE = -1,
        SQUAD_PRIORITY_RESCUE = 0,
        SQUAD_PRIORITY_PUT_IN_BED = 1,
        SQUAD_PRIORITY_MEDIC = 2,
        SQUAD_PRIORITY_ROBOTICS = 3,
        SQUAD_PRIORITY_SPLINTING = 4,
        SQUAD_PRIORITY_ENGINEERING = 5
    };

    struct SquadPriorityResult
    {
        bool success;
        bool interrupted;
        int consideredMembers;
        int changedMembers;
        int unchangedMembers;
        int skippedMembers;
        int skippedUnreadableMembers;
        int movedJobs;
        std::string failedMember;
        std::string message;

        SquadPriorityResult() :
            success(false), interrupted(false), consideredMembers(0),
            changedMembers(0), unchangedMembers(0), skippedMembers(0),
            skippedUnreadableMembers(0), movedJobs(0)
        {
        }
    };

    // The priority batch needs the exact current squad identity and vanilla
    // member order around every mutation, but it does not need every member's
    // name, skills, condition, and queue. Keep that roster check value-only and
    // build a full MemberSnapshot only for the character being changed.
    struct SquadPriorityRosterSnapshot
    {
        HandleIdentity squad;
        std::vector<HandleIdentity> members;
        bool incomplete;

        SquadPriorityRosterSnapshot() : incomplete(false) {}
    };

    int GetSquadPriorityRole(TaskType taskType)
    {
        // The wrapper is the live permanent-job representation of rescue.
        // Treat the direct task as the same family so both remain ahead of
        // putting someone in bed. The original row order remains stable
        // inside this family.
        if (taskType == FIND_AND_RESCUE_IF_THERES_BEDS ||
            taskType == FIND_AND_RESCUE)
        {
            return SQUAD_PRIORITY_RESCUE;
        }
        if (taskType == FIND_BED_AND_PUT_IN)
        {
            return SQUAD_PRIORITY_PUT_IN_BED;
        }
        if (taskType == JOB_MEDIC)
        {
            return SQUAD_PRIORITY_MEDIC;
        }
        if (taskType == JOB_REPAIR_ROBOT)
        {
            return SQUAD_PRIORITY_ROBOTICS;
        }
        if (taskType == SPLINT_JOB)
        {
            return SQUAD_PRIORITY_SPLINTING;
        }
        if (taskType == JOB_BUILDER)
        {
            return SQUAD_PRIORITY_ENGINEERING;
        }
        return SQUAD_PRIORITY_NONE;
    }

    const char* SquadPriorityRoleName(int role)
    {
        switch (role)
        {
        case SQUAD_PRIORITY_RESCUE:
            return "Find and Rescue";
        case SQUAD_PRIORITY_PUT_IN_BED:
            return "Find and Put in Bed";
        case SQUAD_PRIORITY_MEDIC:
            return "Medic";
        case SQUAD_PRIORITY_ROBOTICS:
            return "Robotics";
        case SQUAD_PRIORITY_SPLINTING:
            return "Splinting";
        case SQUAD_PRIORITY_ENGINEERING:
            return "Engineering";
        default:
            return "other jobs";
        }
    }

    bool SameSquadPriorityRoster(
        const SquadPriorityRosterSnapshot& left,
        const SquadPriorityRosterSnapshot& right)
    {
        if (!SameHandleIdentity(left.squad, right.squad) ||
            left.incomplete != right.incomplete ||
            left.members.size() != right.members.size())
        {
            return false;
        }
        for (size_t index = 0; index < left.members.size(); ++index)
        {
            if (!SameHandleIdentity(
                    left.members[index], right.members[index]))
            {
                return false;
            }
        }
        return true;
    }

    bool CanVerifySquadPriorityQueue(
        const std::vector<JobRowSnapshot>& jobs)
    {
        for (size_t index = 0; index < jobs.size(); ++index)
        {
            // Tasker identity is the stable row key. Presentation labels can
            // contain the priority number and therefore change after every
            // move; a zero token has no equally strong fallback here.
            if (jobs[index].taskToken == 0)
            {
                return false;
            }
        }
        return true;
    }

    bool SameSquadPriorityQueueStructure(
        const std::vector<JobRowSnapshot>& left,
        const std::vector<JobRowSnapshot>& right)
    {
        if (left.size() != right.size())
        {
            return false;
        }
        for (size_t index = 0; index < left.size(); ++index)
        {
            if (left[index].taskToken == 0 ||
                right[index].taskToken == 0 ||
                left[index].taskToken != right[index].taskToken ||
                left[index].taskType != right[index].taskType ||
                left[index].hasTarget != right[index].hasTarget ||
                !SameHandleIdentity(left[index].target, right[index].target))
            {
                return false;
            }
        }
        return true;
    }

    int FindSquadPriorityJob(
        const std::vector<JobRowSnapshot>& jobs,
        const JobRowSnapshot& wanted,
        int minimumSlot)
    {
        if (minimumSlot < 0)
        {
            minimumSlot = 0;
        }
        for (size_t index = static_cast<size_t>(minimumSlot);
             index < jobs.size(); ++index)
        {
            if (SameJob(jobs[index], wanted))
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    void InsertSquadPriorityJob(
        std::vector<JobRowSnapshot>* jobs,
        int source,
        int destination)
    {
        if (jobs == NULL || source < 0 || destination < 0 ||
            source >= static_cast<int>(jobs->size()) ||
            destination >= static_cast<int>(jobs->size()) ||
            source == destination)
        {
            return;
        }

        JobRowSnapshot moved = (*jobs)[source];
        jobs->erase(jobs->begin() + source);
        jobs->insert(jobs->begin() + destination, moved);
    }

    bool BuildSquadPriorityOrder(
        const std::vector<JobRowSnapshot>& source,
        std::vector<JobRowSnapshot>* desiredOut)
    {
        if (desiredOut == NULL)
        {
            return false;
        }

        desiredOut->clear();
        desiredOut->reserve(source.size());
        for (int role = SQUAD_PRIORITY_RESCUE;
             role <= SQUAD_PRIORITY_ENGINEERING; ++role)
        {
            for (size_t index = 0; index < source.size(); ++index)
            {
                if (GetSquadPriorityRole(source[index].taskType) == role)
                {
                    desiredOut->push_back(source[index]);
                }
            }
        }
        for (size_t index = 0; index < source.size(); ++index)
        {
            if (GetSquadPriorityRole(source[index].taskType) ==
                SQUAD_PRIORITY_NONE)
            {
                desiredOut->push_back(source[index]);
            }
        }
        return desiredOut->size() == source.size();
    }

    bool TryBuildCurrentSquadPriorityRoster(
        const HandleIdentity& expectedSquad,
        SquadPriorityRosterSnapshot* snapshotOut)
    {
        if (snapshotOut == NULL || !expectedSquad.valid)
        {
            return false;
        }

        hand squadHandle;
        std::string squadName;
        RootObjectContainer* active = NULL;
        if (!TryGetCurrentSquad(
                g_playerInterface, &squadHandle, &squadName, &active) ||
            active == NULL)
        {
            return false;
        }

        SquadPriorityRosterSnapshot snapshot;
        CaptureHandleIdentity(squadHandle, &snapshot.squad);
        if (!SameHandleIdentity(snapshot.squad, expectedSquad))
        {
            return false;
        }

        std::vector<hand> handles;
        if (!TryGetOrderedMemberHandles(
                active, &handles, &snapshot.incomplete))
        {
            return false;
        }
        snapshot.members.reserve(handles.size());
        for (size_t index = 0; index < handles.size(); ++index)
        {
            HandleIdentity member;
            CaptureHandleIdentity(handles[index], &member);
            if (!member.valid || member.type != CHARACTER)
            {
                return false;
            }
            snapshot.members.push_back(member);
        }

        // The ordered-member copy used a borrowed ActivePlatoon pointer only
        // inside its guarded reader. Confirm that the current platoon identity
        // did not change while that copy was made before publishing it.
        hand verifiedHandle;
        std::string verifiedName;
        RootObjectContainer* verifiedActive = NULL;
        if (!TryGetCurrentSquad(
                g_playerInterface, &verifiedHandle,
                &verifiedName, &verifiedActive) ||
            verifiedActive == NULL || verifiedActive != active)
        {
            return false;
        }
        HandleIdentity verifiedIdentity;
        CaptureHandleIdentity(verifiedHandle, &verifiedIdentity);
        if (!SameHandleIdentity(verifiedIdentity, expectedSquad))
        {
            return false;
        }

        *snapshotOut = snapshot;
        return true;
    }

    int FindSquadPriorityRosterMember(
        const SquadPriorityRosterSnapshot& roster,
        const HandleIdentity& member)
    {
        for (size_t index = 0; index < roster.members.size(); ++index)
        {
            if (SameHandleIdentity(roster.members[index], member))
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    bool TryBuildSquadPriorityMember(
        const SquadPriorityRosterSnapshot& roster,
        const HandleIdentity& memberIdentity,
        MemberSnapshot* memberOut)
    {
        if (memberOut == NULL ||
            FindSquadPriorityRosterMember(roster, memberIdentity) < 0)
        {
            return false;
        }

        MemberSnapshot member;
        if (!BuildMemberSnapshot(
                RestoreHandleIdentity(memberIdentity), &member) ||
            !SameHandleIdentity(member.identity, memberIdentity))
        {
            return false;
        }
        *memberOut = member;
        return true;
    }

    void SetSquadPriorityFailure(
        SquadPriorityResult* resultOut,
        const std::string& memberName,
        const char* message)
    {
        if (resultOut == NULL)
        {
            return;
        }
        resultOut->interrupted = true;
        resultOut->failedMember = memberName;
        resultOut->message = message == NULL ? "Priority update interrupted." :
            message;
    }

    bool TryApplySquadPriorityToMember(
        const HandleIdentity& expectedSquad,
        const SquadPriorityRosterSnapshot& expectedMemberOrder,
        const HandleIdentity& memberIdentity,
        SquadPriorityResult* resultOut)
    {
        if (resultOut == NULL)
        {
            return false;
        }

        SquadPriorityRosterSnapshot freshRoster;
        if (!TryBuildCurrentSquadPriorityRoster(
                expectedSquad, &freshRoster) ||
            !SameSquadPriorityRoster(expectedMemberOrder, freshRoster))
        {
            SetSquadPriorityFailure(
                resultOut,
                "",
                "Squad membership changed while priorities were being applied.");
            return false;
        }

        MemberSnapshot freshMember;
        if (!TryBuildSquadPriorityMember(
                freshRoster, memberIdentity, &freshMember))
        {
            ++resultOut->skippedMembers;
            ++resultOut->skippedUnreadableMembers;
            return true;
        }
        if (!freshMember.loaded || !freshMember.queueAvailable ||
            freshMember.truncated)
        {
            ++resultOut->skippedMembers;
            ++resultOut->skippedUnreadableMembers;
            return true;
        }
        if (!CanVerifySquadPriorityQueue(freshMember.jobs))
        {
            ++resultOut->skippedMembers;
            ++resultOut->skippedUnreadableMembers;
            return true;
        }

        std::vector<JobRowSnapshot> desired;
        if (!BuildSquadPriorityOrder(freshMember.jobs, &desired))
        {
            SetSquadPriorityFailure(
                resultOut,
                freshMember.name,
                "A permanent-job queue could not be prepared for priority changes.");
            return false;
        }

        std::vector<JobRowSnapshot> working = freshMember.jobs;
        bool changed = false;
        for (size_t destination = 0; destination < desired.size(); ++destination)
        {
            if (SameJob(working[destination], desired[destination]))
            {
                continue;
            }

            const int source = FindSquadPriorityJob(
                working, desired[destination], static_cast<int>(destination) + 1);
            if (source < 0)
            {
                SetSquadPriorityFailure(
                    resultOut,
                    freshMember.name,
                    "The permanent-job queue changed while priorities were being applied.");
                return false;
            }

            // Validate the exact current platoon and full vanilla member order
            // immediately before every native move. Also re-read the affected
            // queue so an external queue edit cannot be mistaken for our row.
            SquadPriorityRosterSnapshot beforeMoveRoster;
            MemberSnapshot beforeMoveMember;
            if (!TryBuildSquadPriorityMember(
                    expectedMemberOrder,
                    memberIdentity, &beforeMoveMember) ||
                !beforeMoveMember.loaded ||
                !beforeMoveMember.queueAvailable ||
                beforeMoveMember.truncated ||
                !SameSquadPriorityQueueStructure(
                    working, beforeMoveMember.jobs) ||
                !TryBuildCurrentSquadPriorityRoster(
                    expectedSquad, &beforeMoveRoster) ||
                !SameSquadPriorityRoster(
                    expectedMemberOrder, beforeMoveRoster) ||
                FindSquadPriorityRosterMember(
                    beforeMoveRoster, memberIdentity) < 0)
            {
                SetSquadPriorityFailure(
                    resultOut,
                    freshMember.name,
                    "The squad or permanent-job queue changed before a priority move.");
                return false;
            }

            InsertSquadPriorityJob(
                &working, source, static_cast<int>(destination));

            // The guarded engine helper can report false after Kenshi has
            // already moved the row (for example, if the later AI refresh
            // faults). Always verify the full post-move queue before deciding
            // whether the mutation succeeded.
            const bool moveAccepted = TryMovePermajob(
                beforeMoveMember.handle,
                source, static_cast<int>(destination));

            // Verify the complete structural queue after every engine move.
            // Refresh through the current squad so a delayed squad switch or
            // member replacement cannot be mistaken for a successful move.
            SquadPriorityRosterSnapshot verifiedRoster;
            if (!TryBuildCurrentSquadPriorityRoster(
                    expectedSquad, &verifiedRoster) ||
                !SameSquadPriorityRoster(
                    expectedMemberOrder, verifiedRoster))
            {
                SetSquadPriorityFailure(
                    resultOut,
                    freshMember.name,
                    "The squad changed while verifying a priority move.");
                return false;
            }

            MemberSnapshot verifiedMember;
            if (!TryBuildSquadPriorityMember(
                    verifiedRoster, memberIdentity, &verifiedMember) ||
                !verifiedMember.loaded ||
                !verifiedMember.queueAvailable ||
                verifiedMember.truncated ||
                !SameSquadPriorityQueueStructure(
                    working, verifiedMember.jobs))
            {
                SetSquadPriorityFailure(
                    resultOut,
                    freshMember.name,
                    moveAccepted
                        ? "A priority move could not be verified; no further members were changed."
                        : "Kenshi rejected or interrupted a permanent-job priority move; no further members were changed.");
                return false;
            }

            ++resultOut->movedJobs;
            changed = true;
        }

        if (changed)
        {
            ++resultOut->changedMembers;
        }
        else
        {
            ++resultOut->unchangedMembers;
        }
        return true;
    }

    std::string BuildSquadPriorityResultMessage(
        const SquadPriorityResult& result)
    {
        std::ostringstream message;
        if (result.interrupted)
        {
            message << result.message;
            if (!result.failedMember.empty())
            {
                message << " Member: " << result.failedMember << ".";
            }
        }
        else if (result.changedMembers == 0)
        {
            message << "No rescue, medical, robotics, splinting, or engineering jobs needed reordering.";
        }
        else
        {
            message << "Priority order applied to " << result.changedMembers
                    << " squad member" <<
                (result.changedMembers == 1 ? "" : "s") << ".";
        }

        if (result.skippedMembers != 0)
        {
            message << " Skipped " << result.skippedMembers
                    << " unavailable or unreadable member" <<
                (result.skippedMembers == 1 ? "" : "s") << ".";
        }
        return message.str();
    }

    // Apply the requested order to the current/selected squad. Existing rows
    // are reordered only; this action never creates missing jobs. The caller
    // can display result.message without opening a modal dialog.
    bool TryApplySquadPriority(
        const HandleIdentity& expectedSquad,
        SquadPriorityResult* resultOut)
    {
        if (resultOut == NULL)
        {
            return false;
        }
        *resultOut = SquadPriorityResult();
        try
        {
            SquadPriorityRosterSnapshot initial;
            if (!TryBuildCurrentSquadPriorityRoster(expectedSquad, &initial))
            {
                resultOut->interrupted = true;
                resultOut->message =
                    "The selected squad is no longer current or available.";
                return false;
            }
            if (initial.incomplete)
            {
                resultOut->interrupted = true;
                resultOut->message =
                    "The selected squad roster exceeds the safe batch limit. No priorities were changed.";
                return false;
            }

            // Capture the exact member order once. Every member operation then
            // re-reads the squad and validates this identity sequence before it
            // touches a queue. This prevents stale UI handles from crossing a
            // delayed squad switch or roster update.
            for (size_t index = 0; index < initial.members.size(); ++index)
            {
                ++resultOut->consideredMembers;
                if (!TryApplySquadPriorityToMember(
                        expectedSquad,
                        initial,
                        initial.members[index],
                        resultOut))
                {
                    break;
                }
            }

            resultOut->success = !resultOut->interrupted;
            resultOut->message = BuildSquadPriorityResultMessage(*resultOut);
            return resultOut->success;
        }
        catch (...)
        {
            resultOut->success = false;
            resultOut->interrupted = true;
            resultOut->message =
                "Priority update stopped because its snapshot could not be allocated.";
            return false;
        }
    }
