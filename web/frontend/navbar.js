// Jonny Spicer Navbar - Standalone embeddable component
// Modified for subdomain use with absolute URLs
(function() {
  'use strict';

  const baseUrl = 'https://jonnyspicer.com';

  // Menu items with absolute URLs
  const menuItems = [
        {
      name: "Home",
      url: baseUrl + "/",
      title: "Home",
      external: false
    },
    {
      name: "About",
      url: baseUrl + "/about",
      title: "About",
      external: false
    },
    {
      name: "Blog",
      url: baseUrl + "/blog",
      title: "Blog",
      external: false
    },
    {
      name: "Contact",
      url: baseUrl + "/contact",
      title: "Contact",
      external: false
    }
  ];

  // Get the current page path for active state
  function getCurrentPath() {
    return window.location.pathname;
  }

  // Check if a menu item should be marked as active
  function isActive(itemUrl) {
    // On subdomains, nothing should be marked as active
    return false;
  }

  // Create navbar elements using safe DOM methods
  function createNavbar() {
    // Create header
    const header = document.createElement('header');
    header.id = 'jonny-navbar';

    // Create mobile header
    const mobileHeader = document.createElement('div');
    mobileHeader.className = 'mobile-header';

    const toggle = document.createElement('div');
    toggle.className = 'toggle';

    const toggleSpan = document.createElement('span');
    for (let i = 0; i < 3; i++) {
      toggleSpan.appendChild(document.createElement('i'));
    }
    toggle.appendChild(toggleSpan);
    mobileHeader.appendChild(toggle);

    // Create nav
    const nav = document.createElement('nav');

    // Add menu items
    menuItems.forEach(item => {
      const link = document.createElement('a');
      link.href = item.url;
      link.title = item.title;
      link.textContent = item.name;

      if (isActive(item.url)) {
        link.className = 'active';
      }

      if (item.external) {
        link.target = '_blank';
        link.rel = 'noopener noreferrer';
      }

      nav.appendChild(link);
    });

    // Assemble
    header.appendChild(mobileHeader);
    header.appendChild(nav);

    return header;
  }

  // Initialize the navbar
  function initNavbar() {
    // Find the container element
    const container = document.getElementById('jonny-navbar-container');
    const navbar = createNavbar();

    if (container) {
      container.appendChild(navbar);
    } else {
      // Insert at the beginning of body if no container specified
      document.body.insertBefore(navbar, document.body.firstChild);
    }

    // Set up mobile menu toggle
    const toggle = navbar.querySelector('.toggle');
    const nav = navbar.querySelector('nav');

    if (toggle && nav) {
      toggle.addEventListener('click', function() {
        toggle.classList.toggle('is-active');
        nav.classList.toggle('expanded');
      });
    }
  }

  // Wait for DOM to be ready
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initNavbar);
  } else {
    initNavbar();
  }

  // Export for manual initialization if needed
  window.JonnyNavbar = {
    init: initNavbar,
    menuItems: menuItems
  };
})();
