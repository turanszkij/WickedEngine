
return {
    -- Dialog starts here:

    -- 1:
    {
        "What is it? Can't you see I'm busy?",
        action = function()
            conversation.character.mood = Mood.Angry
        end,
        choices = {
            {
                "What are you doing?",
                action = function()
                    conversation.character.next_dialog = 2
                end
            },
            {
                "Sorry to bother you!",
                action = function()
                    conversation:Exit() 
                    conversation.character.next_dialog = 1
                    conversation.character.mood = Mood.Neutral
                end
            },
        }
    },

    -- 2:
    {
        "I'm running round in circles, it's called excercising."
    },

    -- 3:
    {
        "You should try it, it's good for your body and mind.",
        choices = {
            {
                "Do you want to go running together?",
                action = function()
                    conversation.character:Follow(player)
                    conversation.character.next_dialog = 4
                    conversation.character.mood = Mood.Neutral
                end
            },
            {
                "Enjoy yourself then!",
                action = function()
                    conversation.character.next_dialog = 5
                    conversation.character.mood = Mood.Neutral
                end
            },
        }
    },

    -- 4:
    {"Sure, I'm right behind you!", action_after = function() conversation:Exit() conversation.character.next_dialog = 6 end},

    -- 5:
    {"I will!", action_after = function() conversation:Exit() conversation.character.next_dialog = 1 end},

    


    -- While character is following player, conversation will start here:

    -- 6:
    {
        "Why are we stopping?",
        choices = {
            {
                "Are you tired?",
                action = function()
                    conversation.character.next_dialog = 7
                end
            },
            {
                "You were right, this feels good!",
                action = function()
                    conversation.character.next_dialog = 9
                end
            },
            {
                "Stay away from me!",
                action = function()
                    conversation.character:Unfollow()
                    conversation.character:ReturnToPatrol()
                    conversation.character.next_dialog = 8
                    conversation.character.mood = Mood.Angry
                end
            },
        }
    },

    -- 7:
    {"I'm not, now let's go!", action_after = function() conversation:Exit() conversation.character.next_dialog = 6 end},
    -- 8:
    {"Fine, you were boring me anyway.", action_after = function() conversation:Exit() conversation.character.next_dialog = 1 conversation.character.mood = Mood.Neutral end},
    -- 9:
    {"Oh, do you think so too? I'm happy to hear that."},
    {"I was starting to get worried that I'm the only one around here who is into that sort of stuff...", action_after = function() conversation:Exit() conversation.character.next_dialog = 6 end},
}
