/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_antecedent_morph

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>

#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

// Configuration struct per instance
struct behavior_antecedent_morph_config {
  int serial;
  int max_delay_ms;
  struct zmk_behavior_binding normal_binding;
  struct zmk_behavior_binding morph_binding;
  int32_t antecedents_len;
  int32_t antecedents[];
};

// Data struct per instance
struct behavior_antecedent_morph_data {
    struct zmk_behavior_binding *pressed_binding;
};

// Data shared by all instances
static int32_t code_pressed; // most recently pressed key code (with implicit mods, usage page and keycode)
static int64_t time_pressed; // time stamp in milli-seconds of that release

static int antecedent_morph_keycode_state_changed_listener(const zmk_event_t *eh);

ZMK_LISTENER(behavior_antecedent_morph, antecedent_morph_keycode_state_changed_listener);
ZMK_SUBSCRIPTION(behavior_antecedent_morph,zmk_keycode_state_changed);

// Capture all key press and release events in order to record the most recently pressed key code.
// Note that the event structure gives us the keycode (16 bit), the usage page (8 bit) and the implicit modifiers (8 bit),
// but not the explicit modifiers. If the keymap contains the binding "&kp RA(Y)", for example, then right-alt is an
// implicit modifier so that instead of the Y. the special character Ü is sent.
// Whether the user is holding down a shift key at that moment, however, i.e. the explicit modifiers, is not known. We could
// reconstruct this information by tracking the press and release events of the modifier keys (keycodes higher than 0xe0)
// though.
// We here record all key release events of non-modifier keys (keycodes less than 0xe0).
static int antecedent_morph_keycode_state_changed_listener(const zmk_event_t *eh) {
  struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);

  int32_t code = ((ev->implicit_modifiers & 0xff) << 24) | ((ev->usage_page & 0xff) << 16) | (ev->keycode & 0xffff);

  LOG_DBG("%s keycode %d page %d implicit mods %d explicit mods %d code 0x%08x",ev->state ? "down" : "up",ev->keycode,ev->usage_page,ev->implicit_modifiers,ev->explicit_modifiers,code);
  if ((ev->state) && (ev->keycode < 0xe0)) {
    LOG_DBG("code_pressed changes from 0x%08x to 0x%08x",code_pressed,code);
    code_pressed = code;
    time_pressed = ev->timestamp;
  }

  return(ZMK_EV_EVENT_BUBBLE);
}

// When an antecedent morph binding is pressed. we test whether the most recently pressed key code
// is among the configured antecedents and whether the corresponding release event was no more
// than the configured delay time ago.
static int on_antecedent_morph_binding_pressed(struct zmk_behavior_binding *binding,
					       struct zmk_behavior_binding_event event) {
  const struct device *dev = device_get_binding(binding->behavior_dev);
  const struct behavior_antecedent_morph_config *cfg = dev->config;
  struct behavior_antecedent_morph_data *data = dev->data;
  bool morph = false;

  if (data->pressed_binding != NULL) {
    LOG_ERR("Can't press the same antecedent-morph twice");
    return -ENOTSUP;
  }

  LOG_DBG("press serial no. %d when code_pressed 0x%08x delay %dms explicit_mods 0x%02x",cfg->serial,code_pressed,(int32_t)(event.timestamp-time_pressed),zmk_hid_get_explicit_mods());
  for (int i=0;i<cfg->antecedents_len;i++) {
    if (code_pressed == cfg->antecedents[i]) {
      morph = true;
    }
  }
  if ((morph) && ((int32_t)(event.timestamp-time_pressed)) < cfg->max_delay_ms) {
    LOG_DBG("morph condition satisfied");
    data->pressed_binding = (struct zmk_behavior_binding *)&cfg->morph_binding;
  } else {
    data->pressed_binding = (struct zmk_behavior_binding *)&cfg->normal_binding;
  }
  return behavior_keymap_binding_pressed(data->pressed_binding, event);
}

static int on_antecedent_morph_binding_released(struct zmk_behavior_binding *binding,
						struct zmk_behavior_binding_event event) {
  const struct device *dev = device_get_binding(binding->behavior_dev);
  const struct behavior_antecedent_morph_config *cfg = dev->config;
  struct behavior_antecedent_morph_data *data = dev->data;

  if (data->pressed_binding == NULL) {
    LOG_ERR("Antecedent-morph already released");
    return -ENOTSUP;
  }

  LOG_DBG("release serial %d",cfg->serial);

  struct zmk_behavior_binding *pressed_binding = data->pressed_binding;
  data->pressed_binding = NULL;
  int err;
  err = behavior_keymap_binding_released(pressed_binding, event);
  return err;
}

static const struct behavior_driver_api behavior_antecedent_morph_driver_api = {
  .binding_pressed = on_antecedent_morph_binding_pressed,
  .binding_released = on_antecedent_morph_binding_released,
};

static int behavior_antecedent_morph_init(const struct device *dev) {

  const struct behavior_antecedent_morph_config *cfg = dev->config;

  LOG_DBG("serial no. %d has got %d antecedents.",cfg->serial,cfg->antecedents_len);
  for (int i=0; i<cfg->antecedents_len;i++) {
    LOG_DBG("antedecent no. %d is 0x%08x.",i,cfg->antecedents[i]);
  }

  code_pressed = 0;

  return 0;
}

#define _TRANSFORM_ENTRY(idx, node)                                                                     \
    {                                                                                                   \
        .behavior_dev = DT_PROP(DT_INST_PHANDLE_BY_IDX(node, bindings, idx), label),                    \
        .param1 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param1), (0),            \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param1))),                       \
        .param2 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param2), (0),            \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param2))),                       \
    }

// ??? add to the config the <antecedent> and the <max-delay-ms>

#define KP_INST(n)                                                                                      \
    static struct behavior_antecedent_morph_config behavior_antecedent_morph_config_##n = {             \
        .serial = n,                                                                                    \
        .max_delay_ms = DT_INST_PROP(n, max_delay_ms),				                        \
        .normal_binding = _TRANSFORM_ENTRY(0, n),                                                       \
        .morph_binding = _TRANSFORM_ENTRY(1, n),                                                        \
        .antecedents = DT_INST_PROP(n, antecedents),                                                    \
        .antecedents_len = DT_INST_PROP_LEN(n, antecedents),                                            \
    };                                                                                                  \
    static struct behavior_antecedent_morph_data behavior_antecedent_morph_data_##n = {};               \
    DEVICE_DT_INST_DEFINE(n, behavior_antecedent_morph_init, NULL, &behavior_antecedent_morph_data_##n, \
                          &behavior_antecedent_morph_config_##n, APPLICATION,                           \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_antecedent_morph_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif
