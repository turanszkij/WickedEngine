
return {
    -- Dialog starts here:
    {"Hello! Today is a nice day for a walk, isn't it? The sun is shining, the wind blows lightly, and the temperature is just perfect! To be honest, I don't need anything else to be happy."},
    {"I just finished my morning routine and I'm ready for the day. What should I do now...?"},
    {
        "Anything I can do for you?",
        choices = {
            {
                "Follow me!",
                action = function()
                    conversation.character:Follow(player)
                    conversation.character.next_dialog = 4
                end
            },
            {
                "Never mind.",
                action = function()
                    conversation.character.next_dialog = 5
                end
            }
        }
    },

    -- Dialog 4: When chosen [Follow me] or [Just keep following me]
    {"Lead the way!", action_after = function() conversation:Exit() conversation.character.next_dialog = 6 end},

    -- Dialog 5: When chosen [Never mind] - this also modifies mood (expression) and state (anim) while dialog is playing
    {
        "Have a nice day!",
        action = function()
            conversation.character.mood = Mood.Happy
            conversation.character.state = States.WAVE
            conversation.character:SetAnimationAmount(0.1)
        end,
        action_after = function()
            conversation.character.mood = Mood.Neutral
            conversation.character.state = States.IDLE
            conversation.character:SetAnimationAmount(1)
            conversation:Exit() 
            conversation.character.next_dialog = 1 
        end
    },

    -- Dialog 6: After Dialog 4 finished, so character is following player
    {
        "Where are we going?",
        choices = {
            {"Just keep following me.", action = function() conversation.character.next_dialog = 4 end},
            {"Stay here!", action = function() conversation.character:Unfollow() end}
        }
    },
    {"Gotcha!"}, -- After chosen [Stay here]
}
