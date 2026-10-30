#define MAX_SAMSUNG_PANELS 4

struct samsung_panel_map {
    const void *panel_ptr;
    void *vdd_ptr;
};

static struct samsung_panel_map g_samsung_panel_maps[MAX_SAMSUNG_PANELS];

void *samsung_get_panel_private(const void *panel) {
    int i;
    for (i = 0; i < MAX_SAMSUNG_PANELS; i++) {
        if (g_samsung_panel_maps[i].panel_ptr == panel)
            return g_samsung_panel_maps[i].vdd_ptr;
    }
    return NULL;
}

void samsung_set_panel_private(const void *panel, void *vdd) {
    int i;
    for (i = 0; i < MAX_SAMSUNG_PANELS; i++) {
        if (!g_samsung_panel_maps[i].panel_ptr || g_samsung_panel_maps[i].panel_ptr == panel) {
            g_samsung_panel_maps[i].panel_ptr = panel;
            g_samsung_panel_maps[i].vdd_ptr = vdd;
            return;
        }
    }
}
