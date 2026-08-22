// SPDX-License-Identifier: GPL-3.0-only
// Value-only, fail-closed transfer of one permanent-job row (and an optional
// native companion row) between two loaded player characters.
//
// Integration contract:
//   * Include this file after RuntimeAccess.inl.
//   * The UI must first validate that both identities still belong to the
//     loaded player roster and that its presentation snapshots did not change.
//   * Call TryCaptureGeneralJobQueue at drop time and retain both returned
//     value snapshots in the pending action.
//   * Call TryTransferGeneralPermanentJob later on the game thread.
//   * KJM_GENERAL_JOB_TRANSFER_PROBE enables field-test builds.
//   * KJM_GENERAL_JOB_TRANSFER_VERIFIED enables the same path in a release
//     only after the probe matrix in diagnostics/GeneralJobTransferProbe.md
//     has passed.  With neither macro, every mutation fails closed.

    // KenshiLib 0.4.0 documents these VC100 x64 member offsets in Tasker.h.
    // Tasker and TaskData remain incomplete through AITaskSystem.h in this
    // translation unit, so fields without an exported adapter are copied by
    // these named offsets and immediately validated as scalar values.
    const size_t GENERAL_TASKER_PRIORITY_OFFSET = 0x08;
    const size_t GENERAL_TASKER_RESETS_OFFSET = 0x0C;
    const size_t GENERAL_TASKER_LOCATION_OFFSET = 0x58;
    const size_t GENERAL_TASKER_DATA_OFFSET = 0x70;
    const size_t GENERAL_TASK_DATA_PERMAJOB_OFFSET = 0x04;
    const size_t GENERAL_TASK_DATA_FIXED_TARGET_OFFSET = 0x08;
    const size_t GENERAL_TASK_DATA_ASSOCIATED_OFFSET = 0x0C;
    const size_t GENERAL_TASK_DATA_ASSOCIATED_SECONDARY_OFFSET = 0x10;
    const size_t GENERAL_TASK_DATA_KEY_OFFSET = 0x44;
    const size_t GENERAL_TASK_DATA_NEEDS_TARGET_OFFSET = 0x48;

    typedef char GeneralJobFloatLayoutGuard[
        sizeof(float) == 4 ? 1 : -1];
    typedef char GeneralJobTaskTypeLayoutGuard[
        sizeof(TaskType) == 4 ? 1 : -1];
    typedef char GeneralJobPriorityLayoutGuard[
        sizeof(taskPriority) == 4 ? 1 : -1];
    typedef char GeneralJobVectorLayoutGuard[
        sizeof(Ogre::Vector3) == 12 ? 1 : -1];

    struct GeneralJobTaskDataValue
    {
        bool available;
        TaskType key;
        int permaJob;
        bool fixedTarget;
        TaskType associated;
        TaskType associatedSecondary;
        bool needsTarget;

        GeneralJobTaskDataValue() :
            available(false), key(NULL_TASK), permaJob(0),
            fixedTarget(false), associated(NULL_TASK),
            associatedSecondary(NULL_TASK), needsTarget(false)
        {
        }
    };

    struct GeneralJobRowValue
    {
        TaskType taskType;
        hand subject;
        HandleIdentity subjectIdentity;
        Ogre::Vector3 location;
        taskPriority priority;
        bool resetsWhenDone;
        GeneralJobTaskDataValue taskData;

        GeneralJobRowValue() :
            taskType(NULL_TASK), location(0.0f, 0.0f, 0.0f),
            priority(TP_JUST_ACTION), resetsWhenDone(false)
        {
        }
    };

    struct GeneralJobQueueValue
    {
        HandleIdentity member;
        std::vector<GeneralJobRowValue> rows;
    };

    enum GeneralJobTransferCode
    {
        GENERAL_TRANSFER_SUCCESS,
        GENERAL_TRANSFER_DISABLED,
        GENERAL_TRANSFER_INVALID_REQUEST,
        GENERAL_TRANSFER_CAPTURE_FAILED,
        GENERAL_TRANSFER_SOURCE_CHANGED,
        GENERAL_TRANSFER_DESTINATION_CHANGED,
        GENERAL_TRANSFER_DESTINATION_FULL,
        GENERAL_TRANSFER_DUPLICATE,
        GENERAL_TRANSFER_ADD_REJECTED,
        GENERAL_TRANSFER_ADD_UNEXPECTED_REVIEW,
        GENERAL_TRANSFER_INSERT_FAILED_REVIEW,
        GENERAL_TRANSFER_SOURCE_CHANGED_DUPLICATE_REMAINS,
        GENERAL_TRANSFER_REMOVE_FAILED_DUPLICATE_REMAINS,
        GENERAL_TRANSFER_REMOVE_UNEXPECTED_REVIEW
    };

    struct GeneralJobTransferRequest
    {
        GeneralJobQueueValue sourceBefore;
        GeneralJobQueueValue destinationBefore;
        int sourceSlot;
        int destinationGap;

        GeneralJobTransferRequest() : sourceSlot(-1), destinationGap(-1) {}
    };

    struct GeneralJobTransferOutcome
    {
        GeneralJobTransferCode code;
        int transferredRows;
        bool destinationChanged;
        bool sourceChanged;

        GeneralJobTransferOutcome() :
            code(GENERAL_TRANSFER_INVALID_REQUEST), transferredRows(0),
            destinationChanged(false), sourceChanged(false)
        {
        }
    };

    bool SameFloatBits(float left, float right)
    {
        return std::memcmp(&left, &right, sizeof(float)) == 0;
    }

    bool SameGeneralJobLocation(
        const Ogre::Vector3& left,
        const Ogre::Vector3& right)
    {
        return SameFloatBits(left.x, right.x) &&
            SameFloatBits(left.y, right.y) &&
            SameFloatBits(left.z, right.z);
    }

    bool SameGeneralJobTaskData(
        const GeneralJobTaskDataValue& left,
        const GeneralJobTaskDataValue& right)
    {
        if (left.available != right.available)
        {
            return false;
        }
        if (!left.available)
        {
            return true;
        }
        return left.key == right.key &&
            left.permaJob == right.permaJob &&
            left.fixedTarget == right.fixedTarget &&
            left.associated == right.associated &&
            left.associatedSecondary == right.associatedSecondary &&
            left.needsTarget == right.needsTarget;
    }

    // Structural equality deliberately excludes localized/presentation text
    // and Tasker addresses. Both can change without changing a job queue.
    bool SameGeneralJobRowStructure(
        const GeneralJobRowValue& left,
        const GeneralJobRowValue& right)
    {
        return left.taskType == right.taskType &&
            SameHandleIdentity(
                left.subjectIdentity, right.subjectIdentity) &&
            SameGeneralJobLocation(left.location, right.location) &&
            left.priority == right.priority &&
            left.resetsWhenDone == right.resetsWhenDone &&
            SameGeneralJobTaskData(left.taskData, right.taskData);
    }

    bool SameGeneralJobQueue(
        const GeneralJobQueueValue& left,
        const GeneralJobQueueValue& right)
    {
        if (!SameHandleIdentity(left.member, right.member) ||
            left.rows.size() != right.rows.size())
        {
            return false;
        }
        for (size_t index = 0; index < left.rows.size(); ++index)
        {
            if (!SameGeneralJobRowStructure(
                    left.rows[index], right.rows[index]))
            {
                return false;
            }
        }
        return true;
    }

    TaskType GeneralJobSemanticRole(TaskType taskType)
    {
        // The live permanent wrapper and its visible rescue stage are one
        // role. Find-and-put-in-bed remains its own requested role.
        return taskType == FIND_AND_RESCUE_IF_THERES_BEDS ?
            FIND_AND_RESCUE : taskType;
    }

    bool IsGlobalGeneralJobRole(TaskType taskType)
    {
        const TaskType role = GeneralJobSemanticRole(taskType);
        return role == FIND_AND_RESCUE ||
            role == FIND_BED_AND_PUT_IN ||
            role == JOB_MEDIC || role == JOB_REPAIR_ROBOT ||
            role == SPLINT_JOB || role == JOB_BUILDER;
    }

    // Duplicate policy: global roles are unique by semantic role. Fixed-target
    // and ordinary jobs are unique by TaskType plus exact subject handle.
    bool SameGeneralJobSemantic(
        const GeneralJobRowValue& left,
        const GeneralJobRowValue& right)
    {
        if (GeneralJobSemanticRole(left.taskType) !=
            GeneralJobSemanticRole(right.taskType))
        {
            return false;
        }
        if (IsGlobalGeneralJobRole(left.taskType) &&
            IsGlobalGeneralJobRole(right.taskType))
        {
            return true;
        }
        return SameHandleIdentity(
            left.subjectIdentity, right.subjectIdentity);
    }

    bool SameGeneralJobRecreatedPayload(
        const GeneralJobRowValue& source,
        const GeneralJobRowValue& recreated)
    {
        return source.taskType == recreated.taskType &&
            SameHandleIdentity(
                source.subjectIdentity, recreated.subjectIdentity) &&
            SameGeneralJobLocation(source.location, recreated.location) &&
            source.priority == recreated.priority &&
            source.resetsWhenDone == recreated.resetsWhenDone &&
            SameGeneralJobTaskData(source.taskData, recreated.taskData);
    }

    bool IsFiniteGeneralJobFloat(float value)
    {
        unsigned int bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        return (bits & 0x7F800000U) != 0x7F800000U;
    }

    bool IsValidGeneralJobRowValue(const GeneralJobRowValue& row)
    {
        if (row.taskType == NULL_TASK || !row.taskData.available ||
            row.taskData.permaJob == 0 ||
            row.taskData.key != row.taskType ||
            !IsFiniteGeneralJobFloat(row.location.x) ||
            !IsFiniteGeneralJobFloat(row.location.y) ||
            !IsFiniteGeneralJobFloat(row.location.z))
        {
            return false;
        }
        HandleIdentity copiedSubject;
        CaptureHandleIdentity(row.subject, &copiedSubject);
        return SameHandleIdentity(copiedSubject, row.subjectIdentity);
    }

    // This worker owns all temporary objects. The immediate C++ wrapper lets
    // allocation/copy exceptions unwind them; the outer SEH wrapper then
    // handles only native access faults without triggering VC100 C2712.
    // No borrowed pointer leaves this call chain.
    __declspec(noinline) bool CaptureGeneralJobQueueCppUnchecked(
        const HandleIdentity& memberIdentity,
        GeneralJobQueueValue* queueOut)
    {
        if (queueOut == NULL || !memberIdentity.valid ||
            memberIdentity.type != CHARACTER)
        {
            return false;
        }
        const hand member = RestoreHandleIdentity(memberIdentity);
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
        if (faction == NULL || !faction->isThePlayer())
        {
            return false;
        }
        const int count = character->getPermajobCount();
        if (count < 0 || count > MAX_SAFE_JOB_ROWS)
        {
            return false;
        }

        GeneralJobQueueValue captured;
        captured.member = memberIdentity;
        captured.rows.reserve(static_cast<size_t>(count));
        for (int slot = 0; slot < count; ++slot)
        {
            const Tasker* task = character->getPermajobData(slot);
            if (task == NULL)
            {
                return false;
            }
            // Use Kenshi's exported TaskMatch adapter for the subject. Copy
            // only the remaining documented scalar fields by offset. No
            // borrowed pointer is stored in `row`.
            const unsigned char* taskBytes =
                reinterpret_cast<const unsigned char*>(task);
            GeneralJobRowValue row;
            row.taskType = character->getPermajob(slot);
            if (!TryCopyTaskSubject(task, &row.subject))
            {
                return false;
            }
            CaptureHandleIdentity(row.subject, &row.subjectIdentity);
            row.location =
                *reinterpret_cast<const Ogre::Vector3*>(
                    taskBytes + GENERAL_TASKER_LOCATION_OFFSET);
            row.priority = *reinterpret_cast<const taskPriority*>(
                taskBytes + GENERAL_TASKER_PRIORITY_OFFSET);
            row.resetsWhenDone =
                *reinterpret_cast<const bool*>(
                    taskBytes + GENERAL_TASKER_RESETS_OFFSET);
            const unsigned char* dataBytes =
                *reinterpret_cast<const unsigned char* const*>(
                    taskBytes + GENERAL_TASKER_DATA_OFFSET);
            if (dataBytes == NULL)
            {
                return false;
            }
            row.taskData.available = true;
            row.taskData.key = *reinterpret_cast<const TaskType*>(
                dataBytes + GENERAL_TASK_DATA_KEY_OFFSET);
            row.taskData.permaJob = *reinterpret_cast<const int*>(
                dataBytes + GENERAL_TASK_DATA_PERMAJOB_OFFSET);
            row.taskData.fixedTarget = *reinterpret_cast<const bool*>(
                dataBytes + GENERAL_TASK_DATA_FIXED_TARGET_OFFSET);
            row.taskData.associated = *reinterpret_cast<const TaskType*>(
                dataBytes + GENERAL_TASK_DATA_ASSOCIATED_OFFSET);
            row.taskData.associatedSecondary =
                *reinterpret_cast<const TaskType*>(
                    dataBytes +
                    GENERAL_TASK_DATA_ASSOCIATED_SECONDARY_OFFSET);
            row.taskData.needsTarget = *reinterpret_cast<const bool*>(
                dataBytes + GENERAL_TASK_DATA_NEEDS_TARGET_OFFSET);
            if (!IsValidGeneralJobRowValue(row))
            {
                return false;
            }
            captured.rows.push_back(row);
        }
        *queueOut = captured;
        return true;
    }

    __declspec(noinline) bool CaptureGeneralJobQueueCpp(
        const HandleIdentity& memberIdentity,
        GeneralJobQueueValue* queueOut)
    {
        try
        {
            return CaptureGeneralJobQueueCppUnchecked(
                memberIdentity, queueOut);
        }
        catch (...)
        {
            // Let C++ unwind vector/hand temporaries before the outer SEH
            // boundary handles only native access faults.
            return false;
        }
    }

    __declspec(noinline) bool CaptureGeneralJobQueueGuarded(
        const HandleIdentity* member,
        GeneralJobQueueValue* queueOut)
    {
        __try
        {
            return member != NULL &&
                CaptureGeneralJobQueueCpp(*member, queueOut);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool TryCaptureGeneralJobQueue(
        const HandleIdentity& member,
        GeneralJobQueueValue* queueOut)
    {
        if (queueOut == NULL)
        {
            return false;
        }
        GeneralJobQueueValue captured;
        if (!CaptureGeneralJobQueueGuarded(&member, &captured))
        {
            return false;
        }
        *queueOut = captured;
        return true;
    }

    // Imported saves can contain permanent rows that the transfer path must
    // reject (for example permaJob == 0, a key mismatch, or a non-finite
    // location). Invalid-job cleanup still needs a complete, exact scalar
    // fingerprint so it can remove only a proven fixed-target row. This value
    // is cleanup-only. Never use it to recreate or transfer a job.
    struct InvalidCleanupJobRowValue
    {
        ULONG_PTR taskToken;
        TaskType taskType;
        hand subject;
        HandleIdentity subjectIdentity;
        Ogre::Vector3 location;
        taskPriority priority;
        bool resetsWhenDone;
        GeneralJobTaskDataValue taskData;

        InvalidCleanupJobRowValue() :
            taskToken(0), taskType(NULL_TASK),
            location(0.0f, 0.0f, 0.0f),
            priority(TP_JUST_ACTION), resetsWhenDone(false)
        {
        }
    };

    struct InvalidCleanupJobQueueValue
    {
        HandleIdentity member;
        std::vector<InvalidCleanupJobRowValue> rows;
    };

    bool SameInvalidCleanupJobRowFingerprint(
        const InvalidCleanupJobRowValue& left,
        const InvalidCleanupJobRowValue& right)
    {
        return left.taskToken == right.taskToken &&
            left.taskType == right.taskType &&
            SameHandleIdentity(
                left.subjectIdentity, right.subjectIdentity) &&
            SameGeneralJobLocation(left.location, right.location) &&
            left.priority == right.priority &&
            left.resetsWhenDone == right.resetsWhenDone &&
            SameGeneralJobTaskData(left.taskData, right.taskData);
    }

    bool SameInvalidCleanupJobQueueFingerprint(
        const InvalidCleanupJobQueueValue& left,
        const InvalidCleanupJobQueueValue& right)
    {
        if (!SameHandleIdentity(left.member, right.member) ||
            left.rows.size() != right.rows.size())
        {
            return false;
        }
        for (size_t index = 0; index < left.rows.size(); ++index)
        {
            if (!SameInvalidCleanupJobRowFingerprint(
                    left.rows[index], right.rows[index]))
            {
                return false;
            }
        }
        return true;
    }

    void PublishInvalidCleanupJobQueueNoThrow(
        InvalidCleanupJobQueueValue* destination,
        InvalidCleanupJobQueueValue* source)
    {
        destination->member = source->member;
        destination->rows.swap(source->rows);
    }

    __declspec(noinline) bool CaptureInvalidCleanupJobQueueCppUnchecked(
        const HandleIdentity& memberIdentity,
        InvalidCleanupJobQueueValue* queueOut)
    {
        if (queueOut == NULL || !memberIdentity.valid ||
            memberIdentity.type != CHARACTER)
        {
            return false;
        }
        const hand member = RestoreHandleIdentity(memberIdentity);
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
        if (faction == NULL || !faction->isThePlayer())
        {
            return false;
        }
        const int count = character->getPermajobCount();
        if (count < 0 || count > MAX_SAFE_JOB_ROWS)
        {
            return false;
        }

        InvalidCleanupJobQueueValue captured;
        captured.member = memberIdentity;
        captured.rows.reserve(static_cast<size_t>(count));
        for (int slot = 0; slot < count; ++slot)
        {
            const Tasker* task = character->getPermajobData(slot);
            if (task == NULL)
            {
                return false;
            }
            const unsigned char* taskBytes =
                reinterpret_cast<const unsigned char*>(task);
            InvalidCleanupJobRowValue row;
            row.taskToken = reinterpret_cast<ULONG_PTR>(task);
            if (row.taskToken == 0)
            {
                return false;
            }
            for (size_t earlier = 0;
                 earlier < captured.rows.size(); ++earlier)
            {
                if (captured.rows[earlier].taskToken == row.taskToken)
                {
                    return false;
                }
            }
            row.taskType = character->getPermajob(slot);
            if (!TryCopyTaskSubject(task, &row.subject))
            {
                return false;
            }
            CaptureHandleIdentity(row.subject, &row.subjectIdentity);
            row.location =
                *reinterpret_cast<const Ogre::Vector3*>(
                    taskBytes + GENERAL_TASKER_LOCATION_OFFSET);
            row.priority = *reinterpret_cast<const taskPriority*>(
                taskBytes + GENERAL_TASKER_PRIORITY_OFFSET);
            row.resetsWhenDone = *reinterpret_cast<const bool*>(
                taskBytes + GENERAL_TASKER_RESETS_OFFSET);
            const unsigned char* dataBytes =
                *reinterpret_cast<const unsigned char* const*>(
                    taskBytes + GENERAL_TASKER_DATA_OFFSET);
            if (dataBytes == NULL)
            {
                return false;
            }
            row.taskData.available = true;
            row.taskData.key = *reinterpret_cast<const TaskType*>(
                dataBytes + GENERAL_TASK_DATA_KEY_OFFSET);
            row.taskData.permaJob = *reinterpret_cast<const int*>(
                dataBytes + GENERAL_TASK_DATA_PERMAJOB_OFFSET);
            row.taskData.fixedTarget = *reinterpret_cast<const bool*>(
                dataBytes + GENERAL_TASK_DATA_FIXED_TARGET_OFFSET);
            row.taskData.associated = *reinterpret_cast<const TaskType*>(
                dataBytes + GENERAL_TASK_DATA_ASSOCIATED_OFFSET);
            row.taskData.associatedSecondary =
                *reinterpret_cast<const TaskType*>(
                    dataBytes +
                    GENERAL_TASK_DATA_ASSOCIATED_SECONDARY_OFFSET);
            row.taskData.needsTarget = *reinterpret_cast<const bool*>(
                dataBytes + GENERAL_TASK_DATA_NEEDS_TARGET_OFFSET);
            captured.rows.push_back(row);
        }
        PublishInvalidCleanupJobQueueNoThrow(queueOut, &captured);
        return true;
    }

    __declspec(noinline) bool CaptureInvalidCleanupJobQueueCpp(
        const HandleIdentity& memberIdentity,
        InvalidCleanupJobQueueValue* queueOut)
    {
        try
        {
            return CaptureInvalidCleanupJobQueueCppUnchecked(
                memberIdentity, queueOut);
        }
        catch (...)
        {
            return false;
        }
    }

    __declspec(noinline) bool CaptureInvalidCleanupJobQueueGuarded(
        const HandleIdentity* member,
        InvalidCleanupJobQueueValue* queueOut)
    {
        __try
        {
            return member != NULL &&
                CaptureInvalidCleanupJobQueueCpp(*member, queueOut);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool TryCaptureInvalidCleanupJobQueue(
        const HandleIdentity& member,
        InvalidCleanupJobQueueValue* queueOut)
    {
        if (queueOut == NULL)
        {
            return false;
        }
        InvalidCleanupJobQueueValue captured;
        if (!CaptureInvalidCleanupJobQueueGuarded(&member, &captured))
        {
            return false;
        }
        PublishInvalidCleanupJobQueueNoThrow(queueOut, &captured);
        return true;
    }

    __declspec(noinline) bool AddGeneralJobCppUnchecked(
        const HandleIdentity& destination,
        const GeneralJobRowValue& payload)
    {
        if (!destination.valid || destination.type != CHARACTER ||
            !IsValidGeneralJobRowValue(payload))
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
        if (faction == NULL || !faction->isThePlayer())
        {
            return false;
        }

        // Use the same public player-job path as the stable station action.
        // OrdersReceiver::addJob is a lower-level AI entry point. In the live
        // game it can decline a permanent append after the receiver has begun
        // executing jobs, and it rejects the null-subject global jobs used by
        // the healing batch. Character::addJob owns Kenshi's full permanent
        // shift-add path (`shift=true`, `addDontClear=true`).
        //
        // Resolve a target again immediately before the native call. Never
        // retain or pass a RootObject pointer beyond this guarded leaf. An
        // unloaded target therefore fails closed instead of silently changing
        // the exact target requested by the user.
        RootObject* subject = NULL;
        if (payload.subjectIdentity.valid)
        {
            const hand restoredSubject =
                RestoreHandleIdentity(payload.subjectIdentity);
            if (!restoredSubject || !restoredSubject.isValid())
            {
                return false;
            }
            HandleIdentity restoredIdentity;
            CaptureHandleIdentity(restoredSubject, &restoredIdentity);
            if (!SameHandleIdentity(
                    restoredIdentity, payload.subjectIdentity))
            {
                return false;
            }
            subject = restoredSubject.getRootObject();
            if (subject == NULL)
            {
                return false;
            }
        }
        character->addJob(
            payload.taskType, subject, true, true, payload.location);
        character->reThinkCurrentAIAction();
        return true;
    }

    __declspec(noinline) bool AddGeneralJobCpp(
        const HandleIdentity& destination,
        const GeneralJobRowValue& payload)
    {
        try
        {
            return AddGeneralJobCppUnchecked(destination, payload);
        }
        catch (...)
        {
            // C++ copies and engine-thrown standard exceptions unwind here;
            // the outer SEH leaf remains reserved for native access faults.
            return false;
        }
    }

    __declspec(noinline) bool TryAddGeneralJobLeaf(
        const HandleIdentity* destination,
        const GeneralJobRowValue* payload,
        bool* nativeCallReturnedOut)
    {
        if (nativeCallReturnedOut != NULL)
        {
            *nativeCallReturnedOut = false;
        }
        __try
        {
            const bool success = destination != NULL && payload != NULL &&
                AddGeneralJobCpp(*destination, *payload);
            if (nativeCallReturnedOut != NULL)
            {
                *nativeCallReturnedOut = success;
            }
            return success;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool HasGeneralJobSemanticDuplicate(
        const std::vector<GeneralJobRowValue>& haystack,
        const GeneralJobRowValue& needle)
    {
        for (size_t index = 0; index < haystack.size(); ++index)
        {
            if (SameGeneralJobSemantic(haystack[index], needle))
            {
                return true;
            }
        }
        return false;
    }

    __declspec(noinline) bool ValidateGeneralJobSubjectCpp(
        const GeneralJobRowValue& payload)
    {
        if (!payload.subjectIdentity.valid)
        {
            return !static_cast<bool>(payload.subject);
        }
        const hand restored =
            RestoreHandleIdentity(payload.subjectIdentity);
        HandleIdentity restoredIdentity;
        CaptureHandleIdentity(restored, &restoredIdentity);
        // A permanent job can legitimately retain an exact handle for an
        // unloaded target. Do not require live resolution here; the source
        // queue already proved that Kenshi owns this value. Require only an
        // exact scalar round trip before passing the copied hand back to the
        // native permanent-job API.
        return restored &&
            SameHandleIdentity(restoredIdentity, payload.subjectIdentity);
    }

    __declspec(noinline) bool TryValidateGeneralJobSubject(
        const GeneralJobRowValue* payload)
    {
        __try
        {
            return payload != NULL &&
                ValidateGeneralJobSubjectCpp(*payload);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool IsGeneralJobPrefix(
        const GeneralJobQueueValue& before,
        const GeneralJobQueueValue& after)
    {
        if (!SameHandleIdentity(before.member, after.member) ||
            after.rows.size() < before.rows.size())
        {
            return false;
        }
        for (size_t index = 0; index < before.rows.size(); ++index)
        {
            if (!SameGeneralJobRowStructure(
                    before.rows[index], after.rows[index]))
            {
                return false;
            }
        }
        return true;
    }

    void BuildGeneralJobDestinationAfterInsertion(
        const GeneralJobQueueValue& before,
        const std::vector<GeneralJobRowValue>& appended,
        int insertionGap,
        GeneralJobQueueValue* expectedOut)
    {
        expectedOut->member = before.member;
        expectedOut->rows.clear();
        expectedOut->rows.reserve(before.rows.size() + appended.size());
        expectedOut->rows.insert(
            expectedOut->rows.end(), before.rows.begin(),
            before.rows.begin() + insertionGap);
        expectedOut->rows.insert(
            expectedOut->rows.end(), appended.begin(), appended.end());
        expectedOut->rows.insert(
            expectedOut->rows.end(), before.rows.begin() + insertionGap,
            before.rows.end());
    }

    bool TryMoveGeneralJobAndVerify(
        const HandleIdentity& member,
        const GeneralJobQueueValue& before,
        int from,
        int to,
        GeneralJobQueueValue* afterOut)
    {
        GeneralJobQueueValue liveBefore;
        if (!TryCaptureGeneralJobQueue(member, &liveBefore) ||
            !SameGeneralJobQueue(before, liveBefore))
        {
            return false;
        }
        GeneralJobQueueValue expected = before;
        const GeneralJobRowValue moving = expected.rows[from];
        expected.rows.erase(expected.rows.begin() + from);
        expected.rows.insert(expected.rows.begin() + to, moving);

        const bool nativeReturned =
            TryMovePermajob(RestoreHandleIdentity(member), from, to);
        GeneralJobQueueValue after;
        if (!TryCaptureGeneralJobQueue(member, &after) ||
            !SameGeneralJobQueue(expected, after))
        {
            return false;
        }
        *afterOut = after;
        // A raised SEH after a verified mutation is not treated as failure.
        (void)nativeReturned;
        return true;
    }

    bool TryRemoveGeneralJobAndVerify(
        const HandleIdentity& member,
        const GeneralJobQueueValue& before,
        int slot,
        GeneralJobQueueValue* afterOut)
    {
        if (slot < 0 || slot >= static_cast<int>(before.rows.size()))
        {
            return false;
        }
        GeneralJobQueueValue liveBefore;
        if (!TryCaptureGeneralJobQueue(member, &liveBefore) ||
            !SameGeneralJobQueue(before, liveBefore))
        {
            return false;
        }
        GeneralJobQueueValue expected = before;
        expected.rows.erase(expected.rows.begin() + slot);
        const bool nativeReturned =
            TryRemovePermajob(RestoreHandleIdentity(member), slot);
        GeneralJobQueueValue after;
        if (!TryCaptureGeneralJobQueue(member, &after) ||
            !SameGeneralJobQueue(expected, after))
        {
            return false;
        }
        *afterOut = after;
        (void)nativeReturned;
        return true;
    }

    bool IsExpectedGeneralJobAppend(
        const GeneralJobTransferRequest& request,
        const GeneralJobQueueValue& afterAdd,
        int sourceSlot,
        size_t expectedAddedCount,
        std::vector<GeneralJobRowValue>* addedOut)
    {
        if (addedOut == NULL ||
            !IsGeneralJobPrefix(request.destinationBefore, afterAdd))
        {
            return false;
        }
        const size_t addedCount =
            afterAdd.rows.size() - request.destinationBefore.rows.size();
        if (expectedAddedCount < 1 || expectedAddedCount > 2 ||
            addedCount != expectedAddedCount)
        {
            return false;
        }
        const GeneralJobRowValue& sourcePrimary =
            request.sourceBefore.rows[sourceSlot];
        const GeneralJobRowValue& destinationPrimary =
            afterAdd.rows[request.destinationBefore.rows.size()];
        if (!SameGeneralJobRecreatedPayload(
                sourcePrimary, destinationPrimary))
        {
            return false;
        }

        addedOut->assign(
            afterAdd.rows.begin() + request.destinationBefore.rows.size(),
            afterAdd.rows.end());
        if (expectedAddedCount == 2)
        {
            if (sourceSlot + 1 >=
                    static_cast<int>(request.sourceBefore.rows.size()) ||
                request.sourceBefore.rows[sourceSlot + 1].taskType !=
                    sourcePrimary.taskData.associatedSecondary ||
                !SameGeneralJobRecreatedPayload(
                    request.sourceBefore.rows[sourceSlot + 1],
                    (*addedOut)[1]))
            {
                addedOut->clear();
                return false;
            }
        }
        return true;
    }

    bool GeneralJobAddedRowsAreUnique(
        const GeneralJobQueueValue& destinationBefore,
        const std::vector<GeneralJobRowValue>& added)
    {
        for (size_t index = 0; index < added.size(); ++index)
        {
            if (HasGeneralJobSemanticDuplicate(
                    destinationBefore.rows, added[index]))
            {
                return false;
            }
            for (size_t prior = 0; prior < index; ++prior)
            {
                if (SameGeneralJobSemantic(added[prior], added[index]))
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool TryPositionGeneralJobDestination(
        const GeneralJobQueueValue& afterAppend,
        int destinationGap,
        size_t addedCount,
        GeneralJobQueueValue* positionedOut)
    {
        GeneralJobQueueValue current = afterAppend;
        const int originalCount =
            static_cast<int>(afterAppend.rows.size() - addedCount);
        if (destinationGap < originalCount)
        {
            GeneralJobQueueValue afterMove;
            if (!TryMoveGeneralJobAndVerify(
                    afterAppend.member, current, originalCount,
                    destinationGap, &afterMove))
            {
                return false;
            }
            current = afterMove;
            if (addedCount == 2)
            {
                if (!TryMoveGeneralJobAndVerify(
                        afterAppend.member, current, originalCount + 1,
                        destinationGap + 1, &afterMove))
                {
                    return false;
                }
                current = afterMove;
            }
        }
        *positionedOut = current;
        return true;
    }

    GeneralJobTransferCode TryTransferGeneralPermanentJobEnabled(
        const GeneralJobTransferRequest& request,
        GeneralJobTransferOutcome* outcome)
    {
        if (!request.sourceBefore.member.valid ||
            !request.destinationBefore.member.valid ||
            SameHandleIdentity(
                request.sourceBefore.member,
                request.destinationBefore.member) ||
            request.sourceSlot < 0 ||
            request.sourceSlot >=
                static_cast<int>(request.sourceBefore.rows.size()) ||
            request.destinationGap < 0 ||
            request.destinationGap >
                static_cast<int>(request.destinationBefore.rows.size()))
        {
            return GENERAL_TRANSFER_INVALID_REQUEST;
        }

        GeneralJobQueueValue sourceLive;
        GeneralJobQueueValue destinationLive;
        if (!TryCaptureGeneralJobQueue(
                request.sourceBefore.member, &sourceLive))
        {
            return GENERAL_TRANSFER_CAPTURE_FAILED;
        }
        if (!SameGeneralJobQueue(request.sourceBefore, sourceLive))
        {
            return GENERAL_TRANSFER_SOURCE_CHANGED;
        }
        if (!TryCaptureGeneralJobQueue(
                request.destinationBefore.member, &destinationLive))
        {
            return GENERAL_TRANSFER_CAPTURE_FAILED;
        }
        if (!SameGeneralJobQueue(request.destinationBefore, destinationLive))
        {
            return GENERAL_TRANSFER_DESTINATION_CHANGED;
        }
        int sourceSlot = request.sourceSlot;
        int companionPrimarySlot = -1;
        for (size_t candidateIndex = 0;
             candidateIndex < sourceLive.rows.size(); ++candidateIndex)
        {
            const GeneralJobRowValue& possiblePrimary =
                sourceLive.rows[candidateIndex];
            const GeneralJobRowValue& selected = sourceLive.rows[sourceSlot];
            if (static_cast<int>(candidateIndex) != sourceSlot &&
                possiblePrimary.taskData.associatedSecondary ==
                    selected.taskType &&
                SameHandleIdentity(
                    possiblePrimary.subjectIdentity,
                    selected.subjectIdentity) &&
                SameGeneralJobLocation(
                    possiblePrimary.location, selected.location))
            {
                if (companionPrimarySlot >= 0)
                {
                    // Ambiguous companion ownership cannot be reconstructed
                    // exactly. Reject before calling Kenshi.
                    return GENERAL_TRANSFER_CAPTURE_FAILED;
                }
                companionPrimarySlot =
                    static_cast<int>(candidateIndex);
            }
        }
        if (companionPrimarySlot >= 0)
        {
            if (companionPrimarySlot + 1 != sourceSlot)
            {
                // A native companion was separated from its primary. Do not
                // move it alone or try to repair a non-contiguous bundle.
                return GENERAL_TRANSFER_CAPTURE_FAILED;
            }
            // The user grabbed the adjacent companion card. Move the exact
            // primary-plus-secondary bundle instead of orphaning the primary.
            sourceSlot = companionPrimarySlot;
        }

        const GeneralJobRowValue& payload = sourceLive.rows[sourceSlot];
        const TaskType normalizedPrimary =
            payload.taskData.associated == NULL_TASK ?
            payload.taskType : payload.taskData.associated;
        if (normalizedPrimary != payload.taskType)
        {
            // Kenshi's native add path substitutes TaskData::associated before
            // it creates the permanent row. A different type cannot recreate
            // this exact source row, so reject before any mutation.
            return GENERAL_TRANSFER_CAPTURE_FAILED;
        }
        size_t expectedAddedCount = 1;
        if (payload.taskData.associatedSecondary != NULL_TASK)
        {
            if (sourceSlot + 1 >=
                    static_cast<int>(sourceLive.rows.size()) ||
                sourceLive.rows[sourceSlot + 1].taskType !=
                    payload.taskData.associatedSecondary ||
                !SameHandleIdentity(
                    sourceLive.rows[sourceSlot + 1].subjectIdentity,
                    payload.subjectIdentity) ||
                !SameGeneralJobLocation(
                    sourceLive.rows[sourceSlot + 1].location,
                    payload.location))
            {
                return GENERAL_TRANSFER_CAPTURE_FAILED;
            }
            expectedAddedCount = 2;
        }
        if (destinationLive.rows.size() + expectedAddedCount >
            static_cast<size_t>(MAX_SAFE_JOB_ROWS))
        {
            return GENERAL_TRANSFER_DESTINATION_FULL;
        }
        if (HasGeneralJobSemanticDuplicate(destinationLive.rows, payload) ||
            (expectedAddedCount == 2 &&
             HasGeneralJobSemanticDuplicate(
                 destinationLive.rows,
                 sourceLive.rows[sourceSlot + 1])))
        {
            return GENERAL_TRANSFER_DUPLICATE;
        }
        if (!TryValidateGeneralJobSubject(&payload))
        {
            return GENERAL_TRANSFER_CAPTURE_FAILED;
        }

        GeneralJobQueueValue sourceImmediatelyBeforeAdd;
        GeneralJobQueueValue destinationImmediatelyBeforeAdd;
        if (!TryCaptureGeneralJobQueue(
                sourceLive.member, &sourceImmediatelyBeforeAdd) ||
            !SameGeneralJobQueue(sourceLive, sourceImmediatelyBeforeAdd))
        {
            return GENERAL_TRANSFER_SOURCE_CHANGED;
        }
        if (!TryCaptureGeneralJobQueue(
                destinationLive.member, &destinationImmediatelyBeforeAdd) ||
            !SameGeneralJobQueue(
                destinationLive, destinationImmediatelyBeforeAdd))
        {
            return GENERAL_TRANSFER_DESTINATION_CHANGED;
        }

        bool addReturned = false;
        TryAddGeneralJobLeaf(
            &destinationLive.member, &payload, &addReturned);
        GeneralJobQueueValue afterAdd;
        if (!TryCaptureGeneralJobQueue(destinationLive.member, &afterAdd))
        {
            return GENERAL_TRANSFER_ADD_UNEXPECTED_REVIEW;
        }
        if (SameGeneralJobQueue(destinationLive, afterAdd))
        {
            return GENERAL_TRANSFER_ADD_REJECTED;
        }

        std::vector<GeneralJobRowValue> added;
        const bool expectedAppend =
            IsExpectedGeneralJobAppend(
                request, afterAdd, sourceSlot,
                expectedAddedCount, &added);
        const bool uniqueAppend = expectedAppend &&
            GeneralJobAddedRowsAreUnique(destinationLive, added);
        if (!expectedAppend || !uniqueAppend)
        {
            if (added.empty() &&
                IsGeneralJobPrefix(destinationLive, afterAdd))
            {
                const size_t suffixCount =
                    afterAdd.rows.size() - destinationLive.rows.size();
                if (suffixCount >= 1 && suffixCount <= 2)
                {
                    // A strict unchanged prefix proves that this call added
                    // the suffix, even if its semantics were unexpected.
                    added.assign(
                        afterAdd.rows.begin() + destinationLive.rows.size(),
                        afterAdd.rows.end());
                }
            }
            outcome->destinationChanged = true;
            return GENERAL_TRANSFER_ADD_UNEXPECTED_REVIEW;
        }
        outcome->destinationChanged = true;
        outcome->transferredRows = static_cast<int>(added.size());
        (void)addReturned;

        GeneralJobQueueValue positioned;
        if (!TryPositionGeneralJobDestination(
                afterAdd, request.destinationGap,
                added.size(), &positioned))
        {
            outcome->destinationChanged = true;
            return GENERAL_TRANSFER_INSERT_FAILED_REVIEW;
        }
        GeneralJobQueueValue expectedPositioned;
        BuildGeneralJobDestinationAfterInsertion(
            destinationLive, added, request.destinationGap,
            &expectedPositioned);
        if (!SameGeneralJobQueue(expectedPositioned, positioned))
        {
            outcome->destinationChanged = true;
            return GENERAL_TRANSFER_INSERT_FAILED_REVIEW;
        }

        // Revalidate both full queues after every destination mutation and
        // immediately before the first destructive source operation.
        GeneralJobQueueValue sourceBeforeRemove;
        GeneralJobQueueValue destinationBeforeRemove;
        if (!TryCaptureGeneralJobQueue(
                request.sourceBefore.member, &sourceBeforeRemove) ||
            !SameGeneralJobQueue(request.sourceBefore, sourceBeforeRemove) ||
            !TryCaptureGeneralJobQueue(
                request.destinationBefore.member,
                &destinationBeforeRemove) ||
            !SameGeneralJobQueue(
                expectedPositioned, destinationBeforeRemove))
        {
            return GENERAL_TRANSFER_SOURCE_CHANGED_DUPLICATE_REMAINS;
        }

        GeneralJobQueueValue sourceCurrent = sourceBeforeRemove;
        for (size_t removeIndex = added.size(); removeIndex > 0; --removeIndex)
        {
            GeneralJobQueueValue destinationStillPositioned;
            if (!TryCaptureGeneralJobQueue(
                    request.destinationBefore.member,
                    &destinationStillPositioned) ||
                !SameGeneralJobQueue(
                    expectedPositioned, destinationStillPositioned))
            {
                return GENERAL_TRANSFER_REMOVE_UNEXPECTED_REVIEW;
            }
            GeneralJobQueueValue sourceStillCurrent;
            if (!TryCaptureGeneralJobQueue(
                    request.sourceBefore.member, &sourceStillCurrent) ||
                !SameGeneralJobQueue(sourceCurrent, sourceStillCurrent))
            {
                return GENERAL_TRANSFER_SOURCE_CHANGED_DUPLICATE_REMAINS;
            }
            const int slot = sourceSlot +
                static_cast<int>(removeIndex - 1);
            if (slot < 0 || slot >= static_cast<int>(sourceCurrent.rows.size()) ||
                !SameGeneralJobRecreatedPayload(
                    added[removeIndex - 1], sourceCurrent.rows[slot]))
            {
                return GENERAL_TRANSFER_REMOVE_FAILED_DUPLICATE_REMAINS;
            }
            GeneralJobQueueValue sourceAfterRemove;
            if (!TryRemoveGeneralJobAndVerify(
                    sourceCurrent.member, sourceCurrent, slot,
                    &sourceAfterRemove))
            {
                return GENERAL_TRANSFER_REMOVE_UNEXPECTED_REVIEW;
            }
            sourceCurrent = sourceAfterRemove;
            outcome->sourceChanged = true;
        }

        GeneralJobQueueValue destinationFinal;
        if (!TryCaptureGeneralJobQueue(
                request.destinationBefore.member, &destinationFinal) ||
            !SameGeneralJobQueue(expectedPositioned, destinationFinal))
        {
            return GENERAL_TRANSFER_REMOVE_UNEXPECTED_REVIEW;
        }
        return GENERAL_TRANSFER_SUCCESS;
    }

    GeneralJobTransferCode TryTransferGeneralPermanentJob(
        const GeneralJobTransferRequest& request,
        GeneralJobTransferOutcome* outcomeOut)
    {
        GeneralJobTransferOutcome localOutcome;
        GeneralJobTransferOutcome* outcome =
            outcomeOut != NULL ? outcomeOut : &localOutcome;
        *outcome = GeneralJobTransferOutcome();
#if defined(KJM_GENERAL_JOB_TRANSFER_PROBE) || \
    defined(KJM_GENERAL_JOB_TRANSFER_VERIFIED)
        outcome->code =
            TryTransferGeneralPermanentJobEnabled(request, outcome);
#else
        (void)request;
        outcome->code = GENERAL_TRANSFER_DISABLED;
#endif
        return outcome->code;
    }
