    DK_Block* block = DK_GetBlockAt(x, y);

            // Check if a neighboring tile is owned by the same player.
            Side existingJobs = getExistingJobs(block, x, y, player, DK_JOB_CONVERT_WALL);
            if (!(existingJobs & SIDE_NORTH) &&
                    (b = DK_GetBlockAt(x, y - 1)) &&
                    DK_IsBlockPassable(b) &&
                    b->owner == player) {
                // Top is valid.
                DK_Job* job = newJob(player, DK_JOB_CONVERT_WALL);
                job->block = block;
                job->position.d.x = (x + 0.5f);
                job->position.d.y = (y - 0.25f);
            }
            if (!(existingJobs & SIDE_SOUTH) &&
                    (b = DK_GetBlockAt(x, y + 1)) &&
                    DK_IsBlockPassable(b) &&
                    b->owner == player) {
                // Bottom is valid.
                DK_Job* job = newJob(player, DK_JOB_CONVERT_WALL);
                job->block = block;
                job->position.d.x = (x + 0.5f);
                job->position.d.y = (y + 1.25f);
            }
            if (!(existingJobs & SIDE_EAST) &&
                    (b = DK_GetBlockAt(x + 1, y)) &&
                    DK_IsBlockPassable(b) &&
                    b->owner == player) {
                // Right is valid.
                DK_Job* job = newJob(player, DK_JOB_CONVERT_WALL);
                job->block = block;
                job->position.d.x = (x + 1.25f);
                job->position.d.y = (y + 0.5f);
            }
            if (!(existingJobs & SIDE_WEST) &&
                    (b = DK_GetBlockAt(x - 1, y)) &&
                    DK_IsBlockPassable(b) &&
                    b->owner == player) {
                // Left is valid.
                DK_Job* job = newJob(player, DK_JOB_CONVERT_WALL);
                job->block = block;
                job->position.d.x = (x - 0.25f);
                job->position.d.y = (y + 0.5f);
            }
        