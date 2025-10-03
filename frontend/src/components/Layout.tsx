import { Outlet } from 'react-router-dom'
import Navigation from './Navigation'
import BottomPane from './BottomPane'
import { ExitButton } from './ui'

const Layout = () => {
  return (
    <div className="flex h-screen bg-gray-50">
      <Navigation />
      <main className="flex-1 overflow-auto pb-12 relative"> {/* Added relative positioning */}
        {/* EXIT Button - positioned in top right */}
        <div className="absolute top-4 right-6 z-10">
          <ExitButton />
        </div>
        
        <div className="p-6 pt-20"> {/* Added top padding to avoid overlap */}
          <Outlet />
        </div>
      </main>
      <BottomPane />
    </div>
  )
}

export default Layout
