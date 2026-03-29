--[[
    WBP_HUD.lua
    Main HUD Widget Logic

    This script handles the main game HUD including:
    - Health, Armor, Stamina bars
    - Ammo display
    - Damage indicators
    - Status effects
    - Low health warning
]]

local M = UnLua.Class()

-- Configuration
local Config = {
    HealthBarAnimSpeed = 5.0,
    LowHealthPulseSpeed = 2.0,
    DamageIndicatorDuration = 1.0,
    DamageIndicatorFadeSpeed = 2.0,
}

-- Initialize
function M:Initialize(Initializer)
    -- Cache widget references (set in Blueprint)
    -- self.HealthBar
    -- self.ArmorBar
    -- self.StaminaBar
    -- self.AmmoText
    -- self.ReserveAmmoText
    -- self.DamageIndicatorContainer
    -- self.StatusEffectContainer
    -- self.LowHealthOverlay

    -- State
    self.TargetHealthPercent = 1.0
    self.CurrentHealthPercent = 1.0
    self.TargetArmorPercent = 0.0
    self.CurrentArmorPercent = 0.0
    self.TargetStaminaPercent = 1.0
    self.CurrentStaminaPercent = 1.0

    self.DamageIndicators = {}
    self.StatusEffects = {}

    self.LowHealthWarning = false
    self.LowHealthPulseTimer = 0.0
end

-- Called every frame
function M:Tick(MyGeometry, InDeltaTime)
    -- Animate health bar
    self.CurrentHealthPercent = UE.UKismetMathLibrary.FInterpTo(
        self.CurrentHealthPercent,
        self.TargetHealthPercent,
        InDeltaTime,
        Config.HealthBarAnimSpeed
    )
    self:UpdateHealthBarVisual(self.CurrentHealthPercent)

    -- Animate armor bar
    self.CurrentArmorPercent = UE.UKismetMathLibrary.FInterpTo(
        self.CurrentArmorPercent,
        self.TargetArmorPercent,
        InDeltaTime,
        Config.HealthBarAnimSpeed
    )
    self:UpdateArmorBarVisual(self.CurrentArmorPercent)

    -- Animate stamina bar
    self.CurrentStaminaPercent = UE.UKismetMathLibrary.FInterpTo(
        self.CurrentStaminaPercent,
        self.TargetStaminaPercent,
        InDeltaTime,
        Config.HealthBarAnimSpeed
    )
    self:UpdateStaminaBarVisual(self.CurrentStaminaPercent)

    -- Update damage indicators
    self:UpdateDamageIndicators(InDeltaTime)

    -- Low health pulse effect
    if self.LowHealthWarning then
        self.LowHealthPulseTimer = self.LowHealthPulseTimer + InDeltaTime * Config.LowHealthPulseSpeed
        local PulseAlpha = (math.sin(self.LowHealthPulseTimer * math.pi * 2) + 1) * 0.25 + 0.1
        self:SetLowHealthOverlayAlpha(PulseAlpha)
    end
end

-- Health changed callback (called from C++)
function M:OnHealthChanged(CurrentHealth, MaxHealth, Percentage)
    self.TargetHealthPercent = Percentage

    -- Update text if available
    if self.HealthText then
        self.HealthText:SetText(string.format("%.0f / %.0f", CurrentHealth, MaxHealth))
    end
end

-- Armor changed callback
function M:OnArmorChanged(CurrentArmor, MaxArmor, Percentage)
    self.TargetArmorPercent = Percentage

    if self.ArmorText then
        self.ArmorText:SetText(string.format("%.0f", CurrentArmor))
    end

    -- Hide armor bar if no armor
    if self.ArmorBarContainer then
        self.ArmorBarContainer:SetVisibility(
            CurrentArmor > 0 and UE.ESlateVisibility.Visible or UE.ESlateVisibility.Collapsed
        )
    end
end

-- Stamina changed callback
function M:OnStaminaChanged(CurrentStamina, MaxStamina, Percentage)
    self.TargetStaminaPercent = Percentage
end

-- Ammo changed callback
function M:OnAmmoChanged(CurrentMagazine, MaxMagazine, CurrentReserve)
    if self.AmmoText then
        self.AmmoText:SetText(string.format("%d", CurrentMagazine))
    end

    if self.ReserveAmmoText then
        self.ReserveAmmoText:SetText(string.format("/ %d", CurrentReserve))
    end

    -- Flash ammo counter if low
    if CurrentMagazine <= math.floor(MaxMagazine * 0.25) and CurrentMagazine > 0 then
        self:FlashAmmoWarning()
    end
end

-- Crosshair spread changed callback
function M:OnCrosshairSpreadChanged(SpreadAngle)
    -- Forward to crosshair widget if available
    if self.CrosshairWidget then
        self.CrosshairWidget:SetSpread(SpreadAngle)
    end
end

-- Hit marker callback
function M:OnShowHitMarker(bKill)
    if self.CrosshairWidget then
        self.CrosshairWidget:ShowHitMarker(bKill)
    end
end

-- Low health warning callback
function M:OnLowHealthWarning(bShow)
    self.LowHealthWarning = bShow

    if not bShow then
        self.LowHealthPulseTimer = 0.0
        self:SetLowHealthOverlayAlpha(0.0)
    end
end

-- Status effect callback
function M:OnStatusEffectChanged(EffectID, bActive, Icon, RemainingDuration)
    if bActive then
        self:AddStatusEffectIcon(EffectID, Icon, RemainingDuration)
    else
        self:RemoveStatusEffectIcon(EffectID)
    end
end

-- Visual update helpers
function M:UpdateHealthBarVisual(Percent)
    if self.HealthBar then
        self.HealthBar:SetPercent(Percent)

        -- Color based on health
        local Color
        if Percent > 0.5 then
            Color = UE.FLinearColor(0.2, 0.8, 0.2, 1.0) -- Green
        elseif Percent > 0.25 then
            Color = UE.FLinearColor(0.8, 0.8, 0.2, 1.0) -- Yellow
        else
            Color = UE.FLinearColor(0.8, 0.2, 0.2, 1.0) -- Red
        end
        self.HealthBar:SetFillColorAndOpacity(Color)
    end
end

function M:UpdateArmorBarVisual(Percent)
    if self.ArmorBar then
        self.ArmorBar:SetPercent(Percent)
    end
end

function M:UpdateStaminaBarVisual(Percent)
    if self.StaminaBar then
        self.StaminaBar:SetPercent(Percent)
    end
end

function M:SetLowHealthOverlayAlpha(Alpha)
    if self.LowHealthOverlay then
        self.LowHealthOverlay:SetRenderOpacity(Alpha)
    end
end

-- Damage indicator system
function M:ShowDamageIndicator(DamageDirection, DamageAmount)
    local Indicator = {
        Direction = DamageDirection,
        Amount = DamageAmount,
        Timer = Config.DamageIndicatorDuration,
        Alpha = 1.0,
    }
    table.insert(self.DamageIndicators, Indicator)
end

function M:UpdateDamageIndicators(DeltaTime)
    local ToRemove = {}

    for i, Indicator in ipairs(self.DamageIndicators) do
        Indicator.Timer = Indicator.Timer - DeltaTime
        Indicator.Alpha = math.max(0, Indicator.Timer / Config.DamageIndicatorDuration)

        if Indicator.Timer <= 0 then
            table.insert(ToRemove, i)
        end
    end

    -- Remove expired indicators (in reverse order)
    for i = #ToRemove, 1, -1 do
        table.remove(self.DamageIndicators, ToRemove[i])
    end
end

-- Status effect icons
function M:AddStatusEffectIcon(EffectID, Icon, Duration)
    self.StatusEffects[EffectID] = {
        Icon = Icon,
        Duration = Duration,
        Widget = nil -- Created dynamically
    }
    -- Create widget here if StatusEffectContainer is available
end

function M:RemoveStatusEffectIcon(EffectID)
    local Effect = self.StatusEffects[EffectID]
    if Effect and Effect.Widget then
        Effect.Widget:RemoveFromParent()
    end
    self.StatusEffects[EffectID] = nil
end

-- Ammo warning flash
function M:FlashAmmoWarning()
    if self.AmmoText then
        -- Play animation if available
        if self.AmmoFlashAnimation then
            self:PlayAnimation(self.AmmoFlashAnimation)
        end
    end
end

return M
