 /**
     * Initialize a bottom-centered toggle group:
     * - Click to toggle `.is-active`
     * - Programmatic selection via `selectToggle(...)`
     *
     * @param {string|Element} groupRef - selector or element for the group container
     * @param {Object} [opts]
     * @param {string} [opts.activeClass='is-active'] - class to mark active button
     * @returns {{ select: Function, getValue: Function, onChange: Function, destroy: Function }}
     */
    function initToggleGroup(groupRef, opts = {}) {
      const group = typeof groupRef === 'string' ? document.querySelector(groupRef) : groupRef;
      if (!group) throw new Error('Toggle group not found');
      const activeClass = opts.activeClass || 'is-active';

      const buttons = () => Array.from(group.querySelectorAll('.btn'));

      // Click handler: set active & emit change
      const onClick = (e) => {
        const btn = e.target.closest('.btn');
        if (!btn || btn.disabled) return;

        setActive(btn);
        emitChange(btn);
      };

      group.addEventListener('click', onClick);

      function setActive(target, { focus = false, emit = false } = {}) {
        const list = buttons();
        list.forEach(b => b.classList.remove(activeClass));
        target.classList.add(activeClass);
        if (focus) target.focus();
        if (emit) emitChange(target);
      }

      function emitChange(btn) {
        // Standard CustomEvent with .detail.value
        const value = btn.dataset.value ?? buttons().indexOf(btn);
        group.dispatchEvent(new CustomEvent('change', {
          bubbles: true,
          detail: { value, button: btn }
        }));
      }

      /**
       * Programmatically select a button.
       * @param {string|number|Element} selector - data-value (string), index (number), or the button element
       * @param {Object} [options]
       * @param {boolean} [options.emit=true] - whether to dispatch a 'change' event
       * @param {boolean} [options.focus=false] - whether to focus the button
       * @returns {HTMLElement|null} - the selected button or null if not found
       */
      function select(selector, { emit = true, focus = false } = {}) {
        const list = buttons();
        let btn = null;

        if (typeof selector === 'number') {
          selector = selector.toString();
        }

        btn = list.find(b => b.dataset.value === selector) || null;

        if (!btn || btn.disabled) return null;
        setActive(btn, { focus, emit });
        return btn;
      }

      /** Get current value (prefers data-value, otherwise index) */
      function getValue() {
        const list = buttons();
        const current = list.find(b => b.classList.contains(activeClass));
        if (!current) return null;
        return current.dataset.value ?? list.indexOf(current);
      }

      /** Subscribe to changes (returns unsubscribe) */
      function onChange(handler) {
        const wrapped = (e) => handler(e.detail);
        group.addEventListener('change', wrapped);
        return () => group.removeEventListener('change', wrapped);
      }

      /** Cleanup listeners */
      function destroy() {
        group.removeEventListener('click', onClick);
      }

      return { select, getValue, onChange, destroy };
    }

    // --- Example usage ---
    const toggle = initToggleGroup('.js-toggle-group');

    // Programmatically set to 'week' (by data-value)
    toggle.select(1, { emit: true, focus: true });

    // Listen for changes
    const unsubscribe = toggle.onChange(({ value, button }) => {
      console.log('Changed to:', value, 'button text:', button.textContent.trim());
    });

    // Later, if needed:
    // unsubscribe();
    // toggle.destroy();